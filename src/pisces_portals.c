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
#include <linux/mm.h>

#include "pisces.h"
#include "pisces_lcall.h"
#include "enclave.h"
#include "pisces_portals.h"
#include "pagemap.h"
#include "util-hashtable.h"

static u32 xpmem_hash_fn(uintptr_t key) {
    return hash_long(key);
}

static int xpmem_eq_fn(uintptr_t key1, uintptr_t key2) {
    return (key1 == key2);
}


/* Structure to store in hash table */
struct pisces_xpmem_region {
    struct pisces_xpmem_make pisces_make;
    struct mm_struct * mm;              /* Null for Kitten creations */
    struct enclave_aspace aspace;     /* 0 for local creations */
};




static ssize_t
portals_read(struct file * filp, char __user * buffer, size_t length, loff_t * offset ) {
    struct pisces_enclave * enclave = filp->private_data;
    struct pisces_portals * portals = &(enclave->portals);
    ssize_t ret = 0, copy_len = 0;
     
    wait_event_interruptible(portals->user_waitq,
        (portals->ppe_lcall != NULL));

    __asm__ __volatile__ ("":::"memory");
    mb();

    if (!portals->connected) {
        return 0;
    }

    if (!portals->ppe_lcall) {
        /* It could be because we are exiting via ctrl+C */
        return -EFAULT;
    }

    /* Set up the userspace structure */
    if (length > portals->ppe_lcall->lcall.data_len) {
        copy_len = portals->ppe_lcall->lcall.data_len;
    } else {
        copy_len = length;
    }

    if (copy_to_user(buffer, portals->ppe_lcall->lcall.data, copy_len)) {
        printk(KERN_ERR "Error copying Portals command to userspace\n");
        ret = -EFAULT;
        goto err;
    } else {
        ret = copy_len;
    }

    kfree(portals->ppe_lcall);
    portals->ppe_lcall = NULL;
    return ret;

err:
    {
        struct pisces_lcall_resp lcall_resp;
        lcall_resp.status = -1;
        lcall_resp.data_len = 0;
        pisces_xbuf_complete(portals->xbuf_desc, (u8 *)&lcall_resp, 
                sizeof(struct pisces_lcall_resp));
        return ret;
    }
}

static ssize_t 
portals_write(struct file * filp, const char __user * buffer, size_t length, loff_t * offset) {
    struct pisces_enclave * enclave = filp->private_data;
    struct pisces_portals * portals = &(enclave->portals);
    struct pisces_ppe_lcall * ppe_lcall_resp = NULL; 

    ssize_t ret;

    if (!portals->connected) {
        return 0;
    }

    ppe_lcall_resp = kmalloc(sizeof(struct pisces_ppe_lcall) + length, GFP_KERNEL);
    if (!ppe_lcall_resp) {
        printk(KERN_ERR "Unable to allocate buffer for PPE response\n");
        return -ENOMEM;
    }
    
    /* Copy data to portals buffer */
    if (copy_from_user(ppe_lcall_resp->data, buffer, length)) {
        printk(KERN_ERR "Error copying Portals response from userspace\n");
        ppe_lcall_resp->lcall_resp.status = -1;
        ppe_lcall_resp->lcall_resp.data_len = 0;
        ret = -EFAULT;
    } else {
        /* Signal that the result is written */
        ppe_lcall_resp->lcall_resp.status = 0;
        ppe_lcall_resp->lcall_resp.data_len = length;
        ret = length;
    }

    pisces_xbuf_complete(portals->xbuf_desc, (u8 *)ppe_lcall_resp,
            sizeof(struct pisces_ppe_lcall) + length);

    kfree(ppe_lcall_resp);
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


/* These are xpmem commands made by the PPE - some require remote communication,
 * some don't */
static long
portals_ioctl(struct file * filp, unsigned int ioctl, unsigned long arg) {
    struct pisces_enclave * enclave = (struct pisces_enclave *)filp->private_data;
    struct pisces_portals * portals = &(enclave->portals);
    struct pisces_xpmem_region * xpmem_region = NULL;
    long ret = 0;

    /* Always check to make sure the map has been created */
    if (!portals->xpmem_map) {
        printk(KERN_ERR "Cannot handle xpmem ioctl (%d) - failed to init map\n",
                ioctl);
        return -ENOMEM;
    }

    switch (ioctl) {
        case ENCLAVE_IOCTL_XPMEM_VERSION: {
            if (copy_from_user(&(portals->xpmem_info.xpmem_version), (void *)arg,
                        sizeof(struct pisces_xpmem_version))) {
                printk(KERN_ERR "Error copying version buffer from userspace\n");
                ret = -EFAULT;
                break;
            }
            break;
        }
        case ENCLAVE_IOCTL_XPMEM_MAKE: {
            xpmem_region = kmalloc(sizeof(struct pisces_xpmem_region), GFP_KERNEL);
            if (!xpmem_region) {
                printk(KERN_ERR "Error: out of memory!\n");
                ret = -ENOMEM;
                break;
            }

            if (copy_from_user(&(xpmem_region->pisces_make), (void *)arg, 
                        sizeof(struct pisces_xpmem_make))) {
                printk(KERN_ERR "Error copying make buffer from userspace\n");
                ret = -EFAULT;
                break;
            }

            /* Register region in hashtable */
            xpmem_region->mm = current->mm;
            memset(&(xpmem_region->aspace), 0, sizeof(struct enclave_aspace));
            htable_insert(portals->xpmem_map, xpmem_region->pisces_make.segid,
                    (uintptr_t)xpmem_region);

            break;
        }
        case ENCLAVE_IOCTL_XPMEM_REMOVE: {
            struct pisces_xpmem_remove pisces_remove;
            struct pisces_xpmem_region * region;

            if (copy_from_user(&pisces_remove, (void *)arg,
                        sizeof(struct pisces_xpmem_remove))) {
                printk(KERN_ERR "Error copying remove buffer from userspace\n");
                ret = -EFAULT;
                break;
            }

            /* Remove a region from the hashtable */
            region = (struct pisces_xpmem_region *)
                    htable_remove(portals->xpmem_map, pisces_remove.segid, 0); 
            if (region) {
                kfree(region);
                pisces_remove.result = 0;
            } else {
                printk(KERN_ERR "Region not present - this should be impossible\n");
                pisces_remove.result = -1;
            }

            if (copy_to_user((void *)arg, &(pisces_remove),
                        sizeof(struct pisces_xpmem_remove))) {
                printk(KERN_ERR "Error copying remove buffer to userspace\n");
                ret = -EFAULT;
                break;
            }

            break;
        }
        case ENCLAVE_IOCTL_XPMEM_GET: {
            struct pisces_xpmem_get pisces_get;
            struct pisces_xpmem_region * xpmem_region = NULL;

            if (copy_from_user(&(pisces_get), (void *)arg, 
                        sizeof(struct pisces_xpmem_get))) {
                printk(KERN_ERR "Error copying get buffer from userspace\n");
                ret = -EFAULT;
                break;
            }

            /* Lookup region */
            xpmem_region = (struct pisces_xpmem_region *)
                htable_search(portals->xpmem_map, pisces_get.segid);
            if (xpmem_region) {
                pisces_get.apid = pisces_get.segid;
            } else {
                pisces_get.apid = -1;
            }

            if (copy_to_user((void *)arg, &(pisces_get),
                        sizeof(struct pisces_xpmem_get))) {
                printk(KERN_ERR "Error copying get buffer to userspace\n");
                ret = -EFAULT;
                break;
            }

            break;
        }
        case ENCLAVE_IOCTL_XPMEM_RELEASE: {
            struct pisces_xpmem_release pisces_release;
            struct pisces_xpmem_region * region = NULL;

            if (copy_from_user(&(pisces_release), (void *)arg,
                        sizeof(struct pisces_xpmem_release))) {
                printk(KERN_ERR "Error copying release buffer from userspace\n");
                ret = -EFAULT;
                break;
            }

            /* Lookup the apid in the segid map. We don't do anything either way
             * but return whether or not we found it
             */
            region = (struct pisces_xpmem_region *)
                htable_search(portals->xpmem_map, pisces_release.apid);

            if (region) {
                pisces_release.result = 0;
            } else {
                pisces_release.result = -1;
            }
                        
            if (copy_to_user((void *)arg, &(pisces_release),
                        sizeof(struct pisces_xpmem_release))) {
                printk(KERN_ERR "Error copying release buffer to userspace\n");
                ret = -EFAULT;
                break;
            }
            break;
        }
        case ENCLAVE_IOCTL_XPMEM_ATTACH: {
            struct pisces_xpmem_attach_lcall attach_lcall;
            struct pisces_xpmem_region * xpmem_region = NULL;
            struct pisces_xpmem_make * xpmem_src = NULL;
            struct xpmem_pfn * pfns = NULL;
            u64 num_pfns = 0;
            int status = 0;

            if (copy_from_user(&(attach_lcall.pisces_attach), (void *)arg,
                        sizeof(struct pisces_xpmem_attach))) {
                printk(KERN_ERR "Error copying attach buffer from userspace\n");
                ret = -EFAULT;
                break;
            }

            /* Lookup the apid in the segid map */
            xpmem_region = (struct pisces_xpmem_region *)
                htable_search(portals->xpmem_map, attach_lcall.pisces_attach.apid);
            if (!xpmem_region) {
                printk(KERN_ERR "Cannot handle attach request - no region with apid %p found!\n", 
                        (void *)attach_lcall.pisces_attach.apid);
                ret = -EINVAL;
                break;
            }

            xpmem_src = &(xpmem_region->pisces_make);
            if (xpmem_src->size < attach_lcall.pisces_attach.size) {
                printk(KERN_ERR "Cannot handle attach request - requested size too large!\n");
                ret = -EINVAL;
                break;
            }

            /* Lookup pfn map using enclave aspace pointer */
            status = pisces_get_enclave_xpmem_map(xpmem_src, &(attach_lcall.pisces_attach),
                    &(xpmem_region->aspace),
                    &num_pfns,
                    &pfns);
            if (status != 0) {
                printk(KERN_ERR "Cannot handle attach request - failed to get pfn list\n");
                ret = -EINVAL;
                break;
            }

            attach_lcall.pisces_attach.vaddr = pisces_map_xpmem_pfn_range(pfns, num_pfns);
            if (IS_ERR_VALUE(attach_lcall.pisces_attach.vaddr)) {
                printk(KERN_ERR "Cannot handle attach request - failed to map pfn list\n");
                ret = -EINVAL;
                kfree(pfns);
                break;
            }

            if (copy_to_user((void *)arg, &(attach_lcall.pisces_attach),
                        sizeof(struct pisces_xpmem_attach))) {
                printk(KERN_ERR "Error copying attach buffer to userspace\n");
                ret = -EFAULT;
                kfree(pfns);
            }

            break;
        }
        case ENCLAVE_IOCTL_XPMEM_DETACH: {
            struct pisces_xpmem_detach pisces_detach;

            if (copy_from_user(&(pisces_detach), (void *)arg,
                        sizeof(struct pisces_xpmem_detach))) {
                printk(KERN_ERR "Error copying detach buffer from userspace\n");
                ret = -EFAULT;
                break;
            }

            /* TODO: if we really care we can look this up somehow */
            pisces_detach.result = -1;

            if (copy_to_user((void *)arg, &(pisces_detach),
                        sizeof(struct pisces_xpmem_detach))) {
                printk(KERN_ERR "Error copying detach buffer to userspace\n");
                ret = -EFAULT;
                break;
            }
            
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
    .release        = portals_release,
    .unlocked_ioctl = portals_ioctl,
};

int pisces_portals_init(struct pisces_enclave * enclave) {
    struct pisces_portals * portals = &(enclave->portals);

    spin_lock_init(&portals->lock);
    portals->connected = 0;

    init_waitqueue_head(&portals->user_waitq);

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

    spin_lock_irqsave(&portals->lock, flags);
    if (portals->connected == 0) {
        portals->connected = 1;
        acquired = 1;
    }   
    spin_unlock_irqrestore(&portals->lock, flags);


    if (acquired == 0) {
        printk(KERN_ERR "Portals channel already connected\n");
        return portals->fd;
    }   

    portals->fd = anon_inode_getfd("enclave-portals", &portals_fops, enclave, O_RDWR);

    if (portals->fd < 0) {
        printk(KERN_ERR "Error creating Portals inode\n");
    }   

    return portals->fd;
}



/****** LCALL callbacks ********/


/* Lcall handler invoked by lcall kernel thread - wake up any user threads
 * waiting on a portals read */
int
pisces_portals_ppe_message(struct pisces_enclave * enclave, struct pisces_xbuf_desc * xbuf_desc,
        struct pisces_lcall * lcall) {
    struct pisces_portals * portals = &(enclave->portals);
    struct pisces_ppe_lcall * ppe_lcall = (struct pisces_ppe_lcall *)lcall;

    /* Need to copy the lcall to a new buffer, because we are about to return
     * and free the lcall parameter */
    if (portals->ppe_lcall) {
        kfree(portals->ppe_lcall);
    }

    portals->ppe_lcall = kmalloc(sizeof(struct pisces_ppe_lcall) +
            ppe_lcall->lcall.data_len, GFP_KERNEL);
    if (!portals->ppe_lcall) {
        printk(KERN_ERR "Cannot handle ppe LCALL - out of memory!\n");
        return -ENOMEM;
    }

    memcpy(portals->ppe_lcall, ppe_lcall, 
            sizeof(struct pisces_ppe_lcall) + ppe_lcall->lcall.data_len);
    portals->xbuf_desc = xbuf_desc;

    __asm__ __volatile__ ("":::"memory");
    mb();

    wake_up_interruptible(&(portals->user_waitq));

    return 0;
}


/* In these functions, we basically duplicate Kitten libxpmem functionality */

#define INIT_ASPACE_ID 1
#define SMARTMAP_SHIFT 39
static unsigned long make_xpmem_addr(pid_t src, void * vaddr) {
    unsigned long slot;

    printk("Calling make_xpmem_addr for src pid %d\n",
        (int)src);

    if (src == INIT_ASPACE_ID)
        slot = 0;
    else
        slot = src - 0x1000 + 1;

    return (((slot + 1) << SMARTMAP_SHIFT) | ((unsigned long) vaddr));
}

int 
pisces_portals_xpmem_version(
    struct pisces_enclave * enclave,
    struct pisces_xbuf_desc * xbuf_desc,
    struct pisces_lcall * lcall
)
{
    struct pisces_portals * portals = &(enclave->portals);
    struct pisces_xpmem_version_lcall * version_lcall = 
        (struct pisces_xpmem_version_lcall *)lcall;
    struct pisces_xpmem_version_lcall * version_lcall_resp = NULL;
    struct pisces_xpmem_version * pisces_version = NULL;

    if (!portals->xpmem_map) {
        printk(KERN_ERR "Cannot handle enclave xpmem request - failed to init map\n");
        goto err;
    }
    
    version_lcall_resp = version_lcall;
    pisces_version = &(version_lcall->pisces_version);

    pisces_version->version = 0x00022000;

    version_lcall_resp->lcall_resp.status = 0;
    version_lcall_resp->lcall_resp.data_len = 
            sizeof(struct pisces_xpmem_version_lcall) -
            sizeof(struct pisces_lcall);

    pisces_xbuf_complete(xbuf_desc, (u8 *)version_lcall_resp,
            sizeof(struct pisces_xpmem_version_lcall));
    return 0;

err:
    {
        struct pisces_lcall_resp pisces_resp;
        memset(&(pisces_resp), 0, sizeof(struct pisces_resp));
        pisces_resp.status = -1;
        pisces_resp.data_len = 0;
        pisces_xbuf_complete(xbuf_desc, (u8 *)&pisces_resp,
                sizeof(struct pisces_lcall_resp));
        return -1;
    }
}

int 
pisces_portals_xpmem_make(
    struct pisces_enclave * enclave,
    struct pisces_xbuf_desc * xbuf_desc,
    struct pisces_lcall * lcall
)
{
    struct pisces_portals * portals = &(enclave->portals);
    struct pisces_xpmem_make_lcall * make_lcall = 
        (struct pisces_xpmem_make_lcall *)lcall;
    struct pisces_xpmem_make_lcall * make_lcall_resp = NULL;
    struct pisces_xpmem_make * pisces_make = NULL;
    struct pisces_xpmem_region * xpmem_region = NULL;

    if (!portals->xpmem_map) {
        printk(KERN_ERR "Cannot handle enclave xpmem request - failed to init map\n");
        goto err;
    }

    make_lcall_resp = make_lcall;
    pisces_make = &(make_lcall->pisces_make);

    pisces_make->vaddr = (uint64_t)make_xpmem_addr(
            (pid_t)pisces_make->pid,
            (void *)pisces_make->vaddr);
    pisces_make->segid = (int64_t)pisces_make->vaddr;

    /* Insert region in hashtable */
    xpmem_region = kmalloc(sizeof(struct pisces_xpmem_region), GFP_KERNEL);
    if (!xpmem_region) {
        printk(KERN_ERR "Error: out of memory!\n");
        goto err;
    }

    /* Register region in hashtable */
    xpmem_region->pisces_make = *pisces_make;
    xpmem_region->mm = NULL;
    xpmem_region->aspace = make_lcall->aspace;
    htable_insert(portals->xpmem_map, xpmem_region->pisces_make.segid,
                    (uintptr_t)xpmem_region);

    make_lcall_resp->lcall_resp.status = 0;
    make_lcall_resp->lcall_resp.data_len = sizeof(struct pisces_xpmem_make_lcall) -
                sizeof(struct pisces_lcall);

    pisces_xbuf_complete(xbuf_desc, (u8 *)make_lcall_resp,
            sizeof(struct pisces_xpmem_make_lcall)); 
    return 0;

err:
    {
        struct pisces_lcall_resp pisces_resp;
        memset(&(pisces_resp), 0, sizeof(struct pisces_resp));
        pisces_resp.status = -1;
        pisces_resp.data_len = 0;
        pisces_xbuf_complete(xbuf_desc, (u8 *)&pisces_resp,
                sizeof(struct pisces_lcall_resp));
        return -1;
    }
}

int 
pisces_portals_xpmem_remove(
    struct pisces_enclave * enclave,
    struct pisces_xbuf_desc * xbuf_desc,
    struct pisces_lcall * lcall
)
{
    struct pisces_portals * portals = &(enclave->portals);
    struct pisces_xpmem_remove_lcall * remove_lcall = 
        (struct pisces_xpmem_remove_lcall *)lcall;
    struct pisces_xpmem_remove_lcall * remove_lcall_resp = NULL;
    struct pisces_xpmem_remove * pisces_remove = NULL;
    struct pisces_xpmem_region * xpmem_region = NULL;

    if (!portals->xpmem_map) {
        printk(KERN_ERR "Cannot handle enclave xpmem request - failed to init map\n");
        goto err;
    }
    
    remove_lcall_resp = remove_lcall;
    pisces_remove = &(remove_lcall->pisces_remove);

    /* Remove region from hash table */
    xpmem_region = (struct pisces_xpmem_region *)
            htable_remove(portals->xpmem_map, pisces_remove->segid, 0);
    if (xpmem_region) {
        kfree(xpmem_region);
        pisces_remove->result = 0;
    } else {
        printk(KERN_ERR "Region not present - this should be impossible\n");
        pisces_remove->result = -1;
    }

    remove_lcall_resp->lcall_resp.status = 0;
    remove_lcall_resp->lcall_resp.data_len = 
            sizeof(struct pisces_xpmem_remove_lcall) -
            sizeof(struct pisces_lcall);

    pisces_xbuf_complete(xbuf_desc, (u8 *)remove_lcall_resp,
            sizeof(struct pisces_xpmem_remove_lcall));
    return 0;

err:
    {
        struct pisces_lcall_resp pisces_resp;
        memset(&(pisces_resp), 0, sizeof(struct pisces_resp));
        pisces_resp.status = -1;
        pisces_resp.data_len = 0;
        pisces_xbuf_complete(xbuf_desc, (u8 *)&pisces_resp,
                sizeof(struct pisces_lcall_resp));
        return -1;
    }
}

int 
pisces_portals_xpmem_get(
    struct pisces_enclave * enclave,
    struct pisces_xbuf_desc * xbuf_desc,
    struct pisces_lcall * lcall
)
{
    struct pisces_portals * portals = &(enclave->portals);
    struct pisces_xpmem_get_lcall * get_lcall = 
        (struct pisces_xpmem_get_lcall *)lcall;
    struct pisces_xpmem_get_lcall * get_lcall_resp = NULL;
    struct pisces_xpmem_get * pisces_get = NULL;
    struct pisces_xpmem_region * xpmem_region = NULL;

    if (!portals->xpmem_map) {
        printk(KERN_ERR "Cannot handle enclave xpmem request - failed to init map\n");
        goto err;
    }

    get_lcall_resp = get_lcall;
    pisces_get = &(get_lcall->pisces_get);

    xpmem_region = (struct pisces_xpmem_region *)
            htable_search(portals->xpmem_map, pisces_get->segid);
    if (xpmem_region) {
        pisces_get->apid = pisces_get->segid;
    } else {
        printk("Could not find xpmem region (segid = %p)\n",
            (void *)pisces_get->segid);
        pisces_get->apid = -1;
    }

    get_lcall_resp->lcall_resp.status = 0;
    get_lcall_resp->lcall_resp.data_len = 
            sizeof(struct pisces_xpmem_get_lcall) -
            sizeof(struct pisces_lcall);

    pisces_xbuf_complete(xbuf_desc, (u8 *)get_lcall_resp,
            sizeof(struct pisces_xpmem_get_lcall));
    return 0;

err:
    {
        struct pisces_lcall_resp pisces_resp;
        memset(&(pisces_resp), 0, sizeof(struct pisces_resp));
        pisces_resp.status = -1;
        pisces_resp.data_len = 0;
        pisces_xbuf_complete(xbuf_desc, (u8 *)&pisces_resp,
                sizeof(struct pisces_lcall_resp));
        return -1;
    }
}

int 
pisces_portals_xpmem_release(
    struct pisces_enclave * enclave,
    struct pisces_xbuf_desc * xbuf_desc,
    struct pisces_lcall * lcall
)
{
    struct pisces_portals * portals = &(enclave->portals);
    struct pisces_xpmem_release_lcall * release_lcall = 
        (struct pisces_xpmem_release_lcall *)lcall;
    struct pisces_xpmem_release_lcall * release_lcall_resp = NULL;
    struct pisces_xpmem_release * pisces_release = NULL;
    struct pisces_xpmem_region * xpmem_region = NULL;

    release_lcall_resp = release_lcall;
    pisces_release = &(release_lcall->pisces_release);

    if (!portals->xpmem_map) {
        printk(KERN_ERR "Cannot handle enclave xpmem request - failed to init map\n");
        goto err;
    }

    /* Search for region in hash table */
    xpmem_region = (struct pisces_xpmem_region *)
            htable_search(portals->xpmem_map, pisces_release->apid);

    if (xpmem_region) {
        kfree(xpmem_region);
        pisces_release->result = 0;
    } else {
        printk(KERN_ERR "Region not present - this should be impossible\n");
        pisces_release->result = -1;
    }
    
    release_lcall_resp->lcall_resp.status = 0;
    release_lcall_resp->lcall_resp.data_len = 
            sizeof(struct pisces_xpmem_release_lcall) -
            sizeof(struct pisces_lcall);

    pisces_xbuf_complete(xbuf_desc, (u8 *)release_lcall_resp,
            sizeof(struct pisces_xpmem_release_lcall));
    return 0;

err:
    {
        struct pisces_lcall_resp pisces_resp;
        memset(&(pisces_resp), 0, sizeof(struct pisces_resp));
        pisces_resp.status = -1;
        pisces_resp.data_len = 0;
        pisces_xbuf_complete(xbuf_desc, (u8 *)&pisces_resp,
                sizeof(struct pisces_lcall_resp));
        return -1;
    }
}

int 
pisces_portals_xpmem_attach(
    struct pisces_enclave * enclave,
    struct pisces_xbuf_desc * xbuf_desc,
    struct pisces_lcall * lcall
)
{
    struct pisces_portals * portals = &(enclave->portals);
    struct pisces_xpmem_attach_lcall * attach_lcall = 
        (struct pisces_xpmem_attach_lcall *)lcall;
    struct pisces_xpmem_attach_lcall * attach_lcall_resp = NULL;
    struct pisces_xpmem_attach * pisces_attach = NULL;
    struct pisces_xpmem_region * xpmem_region = NULL;

    struct pisces_xpmem_make * xpmem_src = NULL;
    struct xpmem_pfn * pfns = NULL;
    u64 num_pfns = 0;
    u64 pfn_len = 0;
    int status = 0;

    /* Note that we cast out of the command, not the response, as we don't know
     * the response length */
    pisces_attach = &(attach_lcall->pisces_attach);

    if (!portals->xpmem_map) {
        printk(KERN_ERR "Cannot handle enclave xpmem request - failed to init map\n");
        goto err;
    }

    xpmem_region = (struct pisces_xpmem_region *)
            htable_search(portals->xpmem_map, pisces_attach->apid);
    if (!xpmem_region) {
        goto err;
    }

    xpmem_src = &(xpmem_region->pisces_make);
    if (xpmem_src->size < pisces_attach->size) {
        printk(KERN_ERR "Cannot handle attach request - requested size too large!\n");
        goto err;
    }

    printk("Handling xpmem attach request for apid %p\n",
            (void *)(pisces_attach->apid));
    status = pisces_get_xpmem_map(xpmem_src, pisces_attach, xpmem_region->mm,
            &num_pfns,
            &pfns);

    if (status != 0) {
        printk(KERN_ERR "Cannot handle attach request - failed to get pfn list\n");
        goto err;
    }

    printk("num pfns: %llu\n", num_pfns);

    /* Successful pfn lookup */
    pfn_len = sizeof(struct xpmem_pfn) * num_pfns;
    attach_lcall_resp = kmalloc(sizeof(struct pisces_xpmem_attach_lcall) + pfn_len,
            GFP_KERNEL);
    if (!attach_lcall_resp) {
        printk(KERN_ERR "Out of memory\n");
        kfree(pfns);
        goto err;
    }
    
    /* Set up pisces response */
    attach_lcall_resp->num_pfns = num_pfns;
    memcpy(attach_lcall_resp->pfns, pfns, sizeof(struct xpmem_pfn) * num_pfns);

    pisces_attach = &(attach_lcall->pisces_attach);

    attach_lcall_resp->lcall_resp.status = 0;
    attach_lcall_resp->lcall_resp.data_len = 
            sizeof(struct pisces_xpmem_attach_lcall) -
            sizeof(struct pisces_lcall) + pfn_len;

    pisces_xbuf_complete(xbuf_desc, (u8 *)attach_lcall_resp,
            sizeof(struct pisces_xpmem_attach_lcall) + pfn_len);

    kfree(pfns);
    kfree(attach_lcall_resp);
    return 0;

err:
    {
        struct pisces_lcall_resp pisces_resp;
        memset(&(pisces_resp), 0, sizeof(struct pisces_resp));
        pisces_resp.status = -1;
        pisces_resp.data_len = 0;
        pisces_xbuf_complete(xbuf_desc, (u8 *)&pisces_resp,
                sizeof(struct pisces_lcall_resp));
        return -1;
    }
}

int 
pisces_portals_xpmem_detach(
    struct pisces_enclave * enclave,
    struct pisces_xbuf_desc * xbuf_desc,
    struct pisces_lcall * lcall
)
{
    struct pisces_portals * portals = &(enclave->portals);
    struct pisces_xpmem_detach_lcall * detach_lcall = 
        (struct pisces_xpmem_detach_lcall *)lcall;
    struct pisces_xpmem_detach_lcall * detach_lcall_resp = NULL;
    struct pisces_xpmem_detach * pisces_detach = NULL;

    if (!portals->xpmem_map) {
        printk(KERN_ERR "Cannot handle enclave xpmem request - failed to init map\n");
        goto err;
    }
    
    detach_lcall_resp = detach_lcall;
    pisces_detach = &(detach_lcall->pisces_detach);

    /* Not sure we need to do anything here */
    pisces_detach->result = 1;

    detach_lcall_resp->lcall_resp.status = 0;
    detach_lcall_resp->lcall_resp.data_len = 
            sizeof(struct pisces_xpmem_detach_lcall) -
            sizeof(struct pisces_lcall);

    pisces_xbuf_complete(xbuf_desc, (u8 *)detach_lcall_resp,
            sizeof(struct pisces_xpmem_detach_lcall));
    return 0;

err:
    {
        struct pisces_lcall_resp pisces_resp;
        memset(&(pisces_resp), 0, sizeof(struct pisces_resp));
        pisces_resp.status = -1;
        pisces_resp.data_len = 0;
        pisces_xbuf_complete(xbuf_desc, (u8 *)&pisces_resp,
                sizeof(struct pisces_lcall_resp));
        return -1;
    }
}
#if 0
int pisces_portals_xpmem_version(struct pisces_enclave * enclave, struct pisces_cmd * cmd) {
    struct pisces_xpmem_version_cmd * pisces_version_cmd = (struct pisces_xpmem_version_cmd *)cmd;
    struct pisces_xpmem_version_cmd * pisces_version_resp = NULL;
    struct pisces_xpmem_version * pisces_version = NULL;

    pisces_version_resp = pisces_version_cmd;
    pisces_version = &(pisces_version_resp->pisces_version);

    pisces_version->version = 0x00022000;

    pisces_version_resp->resp.status = 0;
    pisces_version_resp->resp.data_len = sizeof(struct pisces_xpmem_version_cmd) -
                sizeof(struct pisces_resp);

    pisces_send_resp(enclave, (struct pisces_resp *)pisces_version_resp);
    return 0;
}

int pisces_portals_xpmem_make(struct pisces_enclave * enclave, struct pisces_cmd * cmd) {
    struct pisces_portals * portals = &(enclave->portals);
    struct pisces_xpmem_make_cmd * pisces_make_cmd = (struct pisces_xpmem_make_cmd *)cmd;
    struct pisces_xpmem_make_cmd * pisces_make_resp = NULL;
    struct pisces_xpmem_make * pisces_make = NULL;
    struct pisces_xpmem_region * xpmem_region = NULL;

    if (!portals->xpmem_map) {
        printk(KERN_ERR "Cannot handle enclave xpmem request - failed to init map\n");
        goto err;
    }

    pisces_make_resp = pisces_make_cmd;
    pisces_make = &(pisces_make_resp->pisces_make);

    pisces_make->vaddr = (uint64_t)make_xpmem_addr(
            (pid_t)pisces_make->pid,
            (void *)pisces_make->vaddr);
    pisces_make->segid = (int64_t)pisces_make->vaddr;

    /* Insert region in hashtable */
    xpmem_region = kmalloc(sizeof(struct pisces_xpmem_region), GFP_KERNEL);
    if (!xpmem_region) {
        printk(KERN_ERR "Error: out of memory!\n");
        goto err;
    }

    /* Register region in hashtable */
    xpmem_region->pisces_make = *pisces_make;
    xpmem_region->mm = NULL;
    htable_insert(portals->xpmem_map, xpmem_region->pisces_make.segid,
                    (uintptr_t)xpmem_region);

    pisces_make_resp->resp.status = 0;
    pisces_make_resp->resp.data_len = sizeof(struct pisces_xpmem_make_cmd) -
                sizeof(struct pisces_resp);

    pisces_send_resp(enclave, (struct pisces_resp *)pisces_make_resp);
    return 0;

err:
    {
        struct pisces_resp pisces_resp;
        memset(&(pisces_resp), 0, sizeof(struct pisces_resp));
        pisces_resp.status = -1;
        pisces_resp.data_len = 0;
        pisces_send_resp(enclave, &pisces_resp);
        return -1;
    }
}

int pisces_portals_xpmem_remove(struct pisces_enclave * enclave, struct pisces_cmd * cmd) {
    struct pisces_portals * portals = &(enclave->portals);
    struct pisces_xpmem_remove_cmd * pisces_remove_cmd = (struct pisces_xpmem_remove_cmd *)cmd;
    struct pisces_xpmem_remove_cmd * pisces_remove_resp = NULL;
    struct pisces_xpmem_remove * pisces_remove = NULL;
    struct pisces_xpmem_region * xpmem_region = NULL;

    if (!portals->xpmem_map) {
        printk(KERN_ERR "Cannot handle enclave xpmem request - failed to init map\n");
        goto err;
    }

    pisces_remove_resp = pisces_remove_cmd;
    pisces_remove = &(pisces_remove_resp->pisces_remove);

    /* Remove region from hashtable */
    xpmem_region = (struct pisces_xpmem_region *)
            htable_remove(portals->xpmem_map, pisces_remove->segid, 0); 
    if (xpmem_region) {
        kfree(xpmem_region);
        pisces_remove->result = 0;
    } else {
        printk(KERN_ERR "Region not present - this should be impossible\n");
        pisces_remove->result = -1;
    }

    pisces_remove_resp->resp.status = 0;
    pisces_remove_resp->resp.data_len = sizeof(struct pisces_xpmem_remove_cmd) -
                sizeof(struct pisces_resp);

    pisces_send_resp(enclave, (struct pisces_resp *)pisces_remove_resp);
    return 0;

err:
    {
        struct pisces_resp pisces_resp;
        memset(&(pisces_resp), 0, sizeof(struct pisces_resp));
        pisces_resp.status = -1;
        pisces_resp.data_len = 0;
        pisces_send_resp(enclave, &pisces_resp);
        return -1;
    }
}

/* Lookup the segid. If present, set apid=segid and return */
int pisces_portals_xpmem_get(struct pisces_enclave * enclave, struct pisces_cmd * cmd) {
    struct pisces_portals * portals = &(enclave->portals);
    struct pisces_xpmem_get_cmd * pisces_get_cmd = (struct pisces_xpmem_get_cmd *)cmd;
    struct pisces_xpmem_get_cmd * pisces_get_resp = NULL;
    struct pisces_xpmem_get * pisces_get = NULL;
    struct pisces_xpmem_region * xpmem_region = NULL;

    pisces_get_resp = pisces_get_cmd;
    pisces_get = &(pisces_get_resp->pisces_get);

    if (!portals->xpmem_map) {
        printk(KERN_ERR "Cannot handle enclave xpmem request - failed to init map\n");
        goto err;
    }

    xpmem_region = (struct pisces_xpmem_region *)
            htable_search(portals->xpmem_map, pisces_get->segid);
    if (xpmem_region) {
        printk("Found xpmem region (segid = %p)\n",
            (void *)pisces_get->segid);
        pisces_get->apid = pisces_get->segid;
    } else {
        printk("Could not find xpmem region (segid = %p) - assuming local get\n",
            (void *)pisces_get->segid);
        pisces_get->apid = -1;
    }

    pisces_get_resp->resp.status = 0;
    pisces_get_resp->resp.data_len = sizeof(struct pisces_xpmem_get_cmd) -
                sizeof(struct pisces_resp);

    pisces_send_resp(enclave, (struct pisces_resp *)pisces_get_resp);
    return 0;

err:
    {
        struct pisces_resp pisces_resp;
        memset(&(pisces_resp), 0, sizeof(struct pisces_resp));
        pisces_resp.status = -1;
        pisces_resp.data_len = 0;
        pisces_send_resp(enclave, &pisces_resp);
        return -1;
    }
}

int pisces_portals_xpmem_release(struct pisces_enclave * enclave, struct pisces_cmd * cmd) {
    struct pisces_portals * portals = &(enclave->portals);
    struct pisces_xpmem_release_cmd * pisces_release_cmd = (struct pisces_xpmem_release_cmd *)cmd;
    struct pisces_xpmem_release_cmd * pisces_release_resp = NULL;
    struct pisces_xpmem_release * pisces_release = NULL;
    struct pisces_xpmem_region * xpmem_region = NULL;

    pisces_release_resp = pisces_release_cmd;
    pisces_release = &(pisces_release_resp->pisces_release);

    if (!portals->xpmem_map) {
        printk(KERN_ERR "Cannot handle enclave xpmem request - failed to init map\n");
        goto err;
    }

    /* Search for region in hashtable */
    xpmem_region = (struct pisces_xpmem_region *)
            htable_search(portals->xpmem_map, pisces_release->apid); 
    if (xpmem_region) {
        kfree(xpmem_region);
        pisces_release->result = 0;
    } else {
        printk(KERN_ERR "Region not present - this should be impossible\n");
        pisces_release->result = -1;
    }

    pisces_release_resp->resp.status = 0;
    pisces_release_resp->resp.data_len = sizeof(struct pisces_xpmem_release_cmd) -
                sizeof(struct pisces_resp);

    pisces_send_resp(enclave, (struct pisces_resp *)pisces_release_resp);
    return 0;

err:
    {
        struct pisces_resp pisces_resp;
        memset(&(pisces_resp), 0, sizeof(struct pisces_resp));
        pisces_resp.status = -1;
        pisces_resp.data_len = 0;
        pisces_send_resp(enclave, &pisces_resp);
        return -1;
    }
}


//FIXME this is broken

/* Lookup the apid. If present, set the pfn map and return */
int pisces_portals_xpmem_attach(struct pisces_enclave * enclave, struct pisces_cmd * cmd) {
    struct pisces_portals * portals = &(enclave->portals);
    struct pisces_xpmem_attach_cmd * pisces_attach_cmd = (struct pisces_xpmem_attach_cmd *)cmd;
    struct pisces_xpmem_attach_cmd * pisces_attach_resp = NULL;
    struct pisces_xpmem_attach * pisces_attach = NULL;
    struct pisces_xpmem_region * xpmem_region = NULL;

    struct pisces_xpmem_make * xpmem_src = NULL;
    struct xpmem_pfn * pfns = NULL;
    u64 num_pfns = 0;
    u64 pfn_len = 0;
    int status = 0;

    /* Note that we cast out of the command, not the response, as we don't know
     * the response length */
    pisces_attach = &(pisces_attach_cmd->pisces_attach);

    if (!portals->xpmem_map) {
        printk(KERN_ERR "Cannot handle enclave xpmem request - failed to init map\n");
        goto err;
    }

    xpmem_region = (struct pisces_xpmem_region *)
            htable_search(portals->xpmem_map, pisces_attach->apid);
    if (!xpmem_region) {
        goto err;
    }

    xpmem_src = &(xpmem_region->pisces_make);
    if (xpmem_src->size < pisces_attach->size) {
        printk("Cannot handle attach request - requested size too large!\n");
        goto err;
    }

    printk("Handling xpmem attach request for apid %p\n", 
            (void *)(pisces_attach->apid));
    status = pisces_get_xpmem_map(xpmem_src, pisces_attach, xpmem_region->mm,
            &num_pfns, 
            &pfns);

    if (status != 0) {
        printk(KERN_ERR "Cannot handle enclave xpmem request - failed to get pfn list\n");
        goto err;
    }

    printk("num pfns: %llu\n", num_pfns);

    /* Successful pfn lookup */
    pfn_len = sizeof(struct xpmem_pfn) * num_pfns;
    pisces_attach_resp = kmalloc(sizeof(struct pisces_xpmem_attach_cmd) + pfn_len, GFP_KERNEL);
    if (!pisces_attach_resp) {
        printk(KERN_ERR "Out of memory\n");
        goto err;
    }

    /* Set up pisces response */
    pisces_attach_resp->num_pfns = num_pfns;
    memcpy(pisces_attach_resp->pfns, pfns, sizeof(struct xpmem_pfn) * num_pfns);

    pisces_attach_resp->resp.status = 0;
    pisces_attach_resp->resp.data_len =  sizeof(struct pisces_xpmem_attach_cmd) - 
            sizeof(struct pisces_resp) +
            pfn_len;

    pisces_send_resp(enclave, (struct pisces_resp *)pisces_attach_resp);
    kfree(pisces_attach_resp);
    return 0;

err:
    {
        struct pisces_resp pisces_resp;
        memset(&(pisces_resp), 0, sizeof(struct pisces_resp));
        pisces_resp.status = -1;
        pisces_resp.data_len = 0;
        pisces_send_resp(enclave, &pisces_resp);
        return -1;
    }
}

/* Not sure we need to do anything here */
int pisces_portals_xpmem_detach(struct pisces_enclave * enclave, struct pisces_cmd * cmd) {
    struct pisces_xpmem_detach_cmd * pisces_detach_cmd = (struct pisces_xpmem_detach_cmd *)cmd;
    struct pisces_xpmem_detach_cmd * pisces_detach_resp = NULL;
    struct pisces_xpmem_detach * pisces_detach = NULL;

    pisces_detach_resp = pisces_detach_cmd;
    pisces_detach = &(pisces_detach_cmd->pisces_detach);

    pisces_detach->result = 1;

    pisces_detach_resp->resp.status = 0;
    pisces_detach_resp->resp.data_len = sizeof(struct pisces_xpmem_detach_cmd) - 
                    sizeof(struct pisces_resp);

    pisces_send_resp(enclave, (struct pisces_resp *)pisces_detach_resp);
    return 0;
}
#endif
