/* 
 * Portals XPMEM communication 
 * (c) Brian Kocoloski, 2013 (briankoco@cs.pitt.edu)
 */

#include <linux/types.h>
#include <linux/sched.h>
#include <linux/anon_inodes.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/kthread.h>

#include "pisces.h"
#include "pisces_lcall.h"
#include "enclave.h"
#include "pisces_cmds.h"
#include "pisces_portals.h"
#include "pisces_data.h"
#include "pagemap.h"

static u32 xpmem_hash_fn(uintptr_t key) {
    return hash_long(key);
}

static int xpmem_eq_fn(uintptr_t key1, uintptr_t key2) {
    return (key1 == key2);
}


/* Structure to store in hash table */
struct pisces_xpmem_region {
    struct pisces_xpmem_make pisces_make;
    struct mm_struct * mm;
};



static ssize_t
portals_read(struct file * filp, char __user * buffer, size_t length, loff_t * offset ) {
    struct pisces_enclave * enclave = filp->private_data;
    struct pisces_portals * portals = &(enclave->portals);
    struct pisces_lcall * lcall_state = &(enclave->lcall);
    struct pisces_cmd_buf * cmd_buf = lcall_state->cmd_buf;
    struct pisces_resp * resp = &(cmd_buf->resp);

    //struct portals_ppe_cmd * ppe_cmd = (struct portals_ppe_cmd *)&(cmd_buf->cmd);
    struct portals_ppe_cmd * ppe_cmd = NULL;
    ssize_t ret = 0, copy_len = 0;

     
    /* Put the process to sleep unless data is already written by the enclave */
    wait_event_interruptible(lcall_state->user_waitq,
        (cmd_buf->active == 1));

    __asm__ __volatile__ ("":::"memory");
    ppe_cmd = (struct portals_ppe_cmd *)lcall_state->cmd;

    if (!portals->connected) {
        return 0;
    }

    if (!ppe_cmd) {
        /* It could be because we are exiting via ctrl+C */
        return -EFAULT;
    }

    /* Set up the userspace structure */
    if (length > ppe_cmd->cmd.data_len) {
        copy_len = ppe_cmd->cmd.data_len;
    } else {
        copy_len = length;
    }

    if (copy_to_user(buffer, ppe_cmd->data, copy_len)) {
        printk(KERN_ERR "Error copying Portals command to userspace\n");
        resp->status = -1;
        resp->data_len = 0;
        ret = -1;
        cmd_buf->completed = 1;
    } else {
        ret = copy_len;
    }

    kfree(lcall_state->cmd);
    lcall_state->cmd = NULL;

    return ret;
}

static ssize_t 
portals_write(struct file * filp, const char __user * buffer, size_t length, loff_t * offset) {
    struct pisces_enclave * enclave = filp->private_data;
    struct pisces_portals * portals = &(enclave->portals);
    struct portals_ppe_cmd * ppe_resp = NULL; 

    ssize_t ret;

    if (!portals->connected) {
        return 0;
    }

    ppe_resp = (struct portals_ppe_cmd *)kmalloc(sizeof(struct portals_ppe_cmd) + length, GFP_KERNEL);
    if (!ppe_resp) {
        printk(KERN_ERR "Unable to allocate buffer for PPE response\n");
        return -ENOMEM;
    }
    
    /* Copy data to portals buffer */
    if (copy_from_user(ppe_resp->data, buffer, length)) {
        printk(KERN_ERR "Error copying Portals response from userspace\n");
        ppe_resp->resp.status = -1;
        ppe_resp->resp.data_len = 0;
        ret = -1;
    } else {
        /* Signal that the result is written */
        ppe_resp->resp.status = length;
        ppe_resp->resp.data_len = length;
        ret = length;
    }

    pisces_send_resp(enclave, (struct pisces_resp *)ppe_resp);

    kfree(ppe_resp);
    return ret;
}

static int
portals_release(struct inode * inodep, struct file * filp) {
    struct pisces_enclave * enclave = filp->private_data;
    struct pisces_portals * portals = &(enclave->portals);
    unsigned long flags = 0;
    int released;

    spin_lock_irqsave(&portals->lock, flags);
    if (portals->connected == 1) {
        portals->connected = 0;
        released = 1;
    }
    spin_unlock_irqrestore(&portals->lock, flags);

    if (released == 0) {
        printk(KERN_ERR "Portals channel prematurely released\n");
    }

    return 0;
}

static long
portals_ioctl(struct file * filp, unsigned int ioctl, unsigned long arg) {
    struct pisces_enclave * enclave = (struct pisces_enclave *)filp->private_data;
    struct pisces_portals * portals = &(enclave->portals);
    struct pisces_xpmem_region * xpmem_region = NULL;
    long ret = 0;

    switch (ioctl) {
        case ENCLAVE_IOCTL_XPMEM_MAKE: {
            xpmem_region = kmalloc(sizeof(struct pisces_xpmem_region), GFP_KERNEL);
            if (!xpmem_region) {
                printk(KERN_ERR "Error: out of memory!\n");
                ret = -ENOMEM;
                break;
            }

            if (copy_from_user(&(xpmem_region->pisces_make), (void *)arg, 
                        sizeof(struct pisces_xpmem_make))) {
                printk(KERN_ERR "Error copying buffer from userspace\n");
                ret = -EINVAL;
                break;
            }

            /* Register region in hashtable */
            xpmem_region->mm = current->mm;
            htable_insert(portals->xpmem_map, xpmem_region->pisces_make.segid,
                    (uintptr_t)xpmem_region);

            ret = xpmem_region->pisces_make.segid;
            break;
        }
        case ENCLAVE_IOCTL_XPMEM_GET: {
            struct pisces_xpmem_get pisces_get;

            if (copy_from_user(&(pisces_get), (void *)arg, sizeof(struct pisces_xpmem_get))) {
                printk(KERN_ERR "Error copying buffer from userspace\n");
                ret = -EINVAL;
                break;
            }

            /* Just set apid=segid - the resulting attach will fail if the
             * segid was invalid */
            pisces_get.apid = pisces_get.segid;

            if (copy_to_user((void *)arg, &(pisces_get), sizeof(struct pisces_xpmem_get))) {
                printk(KERN_ERR "Error copying buffer to userspace\n");
                return -EFAULT;
            }  
            break;
        }
        case ENCLAVE_IOCTL_XPMEM_ATTACH: {
            break;
        }
        default: {
            printk(KERN_ERR "Unhandled ioctl (%d)\n", ioctl);
            ret = -EINVAL;
            break;
        }
    }

    return ret;
}


static struct file_operations portals_fops = {
    .write          = portals_write,
    .read           = portals_read,
    .unlocked_ioctl = portals_ioctl,
    .release        = portals_release,
};


int pisces_portals_xpmem_attach(struct pisces_enclave * enclave, struct pisces_cmd * cmd) {
    struct pisces_portals * portals = &(enclave->portals);
    struct pisces_xpmem_attach_cmd * pisces_attach_cmd = (struct pisces_xpmem_attach_cmd *)cmd;
    struct pisces_xpmem_attach_cmd * pisces_attach_resp = NULL;
    struct pisces_xpmem_region * xpmem_region = NULL;

    struct pisces_xpmem_make * xpmem_src = NULL;
    struct pisces_xpmem_attach * xpmem_dest = NULL;
    struct xpmem_pfn * pfns = NULL;
    int ret = 0;
    u64 num_pfns = 0;
    u64 alloc_len = 0;

    if (!portals->xpmem_map) {
        printk(KERN_ERR "Cannot handle enclave xpmem request - failed to init map\n");
        ret = -1;
        goto err;
    }

    xpmem_dest = &(pisces_attach_cmd->pisces_attach);

    xpmem_region = (struct pisces_xpmem_region *)
            htable_search(portals->xpmem_map, xpmem_dest->apid);

    if (!xpmem_region) {
        printk(KERN_ERR "Cannot handle enclave xpmem request - apid %lli does not exist\n",
                (long long)xpmem_dest->apid);
        ret = -1;
        goto err;
    }

    xpmem_src = &(xpmem_region->pisces_make);

    if (xpmem_src->size < xpmem_dest->size) {
        printk(KERN_ERR "Cannot handle enclave xpmem requset - requested size is too large!\n");
        ret = -1;
        goto err;
    }

    printk("Handling xpmem attach request for apid %lli\n", (long long)(xpmem_dest->apid));
    ret = pisces_get_xpmem_map(xpmem_src, xpmem_dest, xpmem_region->mm,
            &num_pfns, 
            &pfns);

    if (ret != 0) {
        printk(KERN_ERR "Cannot handle enclave xpmem request - failed to get pfn list\n");
        ret = -1;
        goto err;
    }

    /* Successful pfn lookup */
    alloc_len = sizeof(struct xpmem_pfn) * num_pfns;
    pisces_attach_resp = kmalloc(sizeof(struct pisces_xpmem_attach) + alloc_len, GFP_KERNEL);
    if (!pisces_attach_resp) {
        printk(KERN_ERR "Out of memory\n");
        ret = -1;
        goto err;
    }

    /* Set up pisces response */
    pisces_attach_resp->resp.status = 0;
    pisces_attach_resp->resp.data_len =  sizeof(struct pisces_xpmem_attach_cmd) - 
            sizeof(struct pisces_resp) +
            alloc_len;
    pisces_attach_resp->num_pfns = num_pfns;
    memcpy(pisces_attach_resp->pfns, pfns, sizeof(struct xpmem_pfn) * num_pfns);

    pisces_send_resp(enclave, (struct pisces_resp *)pisces_attach_resp);
    return 0;

err:
    {
        struct pisces_resp pisces_resp;
        pisces_resp.status = -1;
        pisces_resp.data_len = 0;
        pisces_send_resp(enclave, &pisces_resp);
        return ret;
    }
}


int pisces_portals_init(struct pisces_enclave * enclave) {
    struct pisces_portals * portals = &(enclave->portals);

    spin_lock_init(&portals->lock);
    portals->connected = 0;

    portals->xpmem_map = create_htable(0, xpmem_hash_fn, xpmem_eq_fn);
    if (!portals->xpmem_map) {
        printk(KERN_ERR "Could not allocate xpmem map\n");
        return -1;
    }

    return 0;
}


int pisces_portals_connect(struct pisces_enclave * enclave) {
    struct pisces_portals * portals = &(enclave->portals);
    unsigned long flags = 0;
    int acquired = 0;
    int portals_fd = 0;

    spin_lock_irqsave(&portals->lock, flags);
    if (portals->connected == 0) {
        portals->connected = 1;
        acquired = 1;
    }   
    spin_unlock_irqrestore(&portals->lock, flags);


    if (acquired == 0) {
        printk(KERN_ERR "Portals channel already connected\n");
        return -1; 
    }   

    portals_fd = anon_inode_getfd("enclave-portals", &portals_fops, enclave, O_RDWR);

    if (portals_fd < 0) {
        printk(KERN_ERR "Error creating Portals inode\n");
        return portals_fd;
    }   

    return portals_fd;
}

