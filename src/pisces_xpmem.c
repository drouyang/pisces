/* 
 * XPMEM communication channel
 *
 * All we're really doing is forwarding commands between the Linux XPMEM
 * name server and the enclave
 *
 * (c) Brian Kocoloski, 2013 (briankoco@cs.pitt.edu)
 */

#include <linux/types.h>
#include <linux/anon_inodes.h>
#include <linux/slab.h>
#include <linux/poll.h>

#include "pisces.h"
#include "pisces_boot_params.h"
#include "pisces_lcall.h"
#include "pisces_xbuf.h"
#include "enclave.h"


/* We use the XPMEM control channel to send requests/responses */
static int 
pisces_xpmem_cmd_ctrl(struct pisces_enclave        * enclave, 
		      struct pisces_xpmem_cmd_ctrl * xpmem_ctrl, 
		      u64                            size) 
{
    struct pisces_xpmem     * xpmem     = &(enclave->xpmem);
    struct pisces_xbuf_desc * xbuf_desc = xpmem->xbuf_desc;

    return pisces_xbuf_send(xbuf_desc, (u8 *)xpmem_ctrl, size);
}


static int
xpmem_open(struct inode * inodep, 
	   struct file  * filp) 
{
    return 0;
}

static int
xpmem_release(struct inode * inodep, 
	      struct file  * filp) 
{
    struct pisces_enclave * enclave = (struct pisces_enclave *)filp->private_data;
    struct pisces_xpmem   * xpmem   = &(enclave->xpmem);

    unsigned long flags;

    if (!xpmem->initialized) {
        return -EBADF;
    }

    spin_lock_irqsave(&(xpmem->state_lock), flags);
    {
	xpmem->fd = -1;
    }
    spin_unlock_irqrestore(&(xpmem->state_lock), flags);

    return 0;
}

static ssize_t
xpmem_read(struct file * filp,
	   char __user * buffer,
	   size_t        size, 
	   loff_t      * offp) 
{
    struct pisces_enclave * enclave = (struct pisces_enclave *)filp->private_data;
    struct pisces_xpmem   * xpmem   = &(enclave->xpmem);
    struct xpmem_cmd_iter * iter    = NULL;
    ssize_t ret = size;
    unsigned long flags;

    if (!xpmem->initialized) {
        return -EBADF;
    }

    if (size != (sizeof(struct xpmem_cmd_ex))) {
        return -EINVAL;
    }

    spin_lock_irqsave(&(xpmem->in_lock), flags);
    {
	if (!list_empty(&(xpmem->in_cmds))) {
	    iter = list_first_entry(&(xpmem->in_cmds), struct xpmem_cmd_iter, node);
	    list_del(&(iter->node));
	}
	
    }
    spin_unlock_irqrestore(&(xpmem->in_lock), flags);

    if (iter == NULL) {
	/* List was empty */
	return 0;
    }
    
    if (copy_to_user(buffer, (void *)(iter->cmd), size)) {
        ret = -EFAULT;
    }

    kfree(iter->cmd);
    kfree(iter);
    return ret;
}

static ssize_t 
xpmem_write(struct file       * filp, 
	    const char __user * buffer, 
	    size_t              size, 
	    loff_t            * offp) 
{
    struct pisces_enclave        * enclave = (struct pisces_enclave *)filp->private_data;
    struct pisces_xpmem          * xpmem   = &(enclave->xpmem);
    struct pisces_xpmem_cmd_ctrl * ctrl    =  NULL;
    struct xpmem_cmd_ex            cmd_ex;

    int     status  = 0;
    u64     pfn_len = 0;
    ssize_t ret     = size;

    if (!xpmem->initialized) {
        return -EBADF;
    }

    if (size != sizeof(struct xpmem_cmd_ex)) {
        return -EINVAL;
    }

    if (copy_from_user((void *)&(cmd_ex), buffer, size)) {
        return -EFAULT;
    }

    if (cmd_ex.type == XPMEM_ATTACH_COMPLETE) {
        pfn_len = cmd_ex.attach.num_pfns * sizeof(u64);
    }

    switch (cmd_ex.type) {
        case XPMEM_GET:
        case XPMEM_RELEASE:
        case XPMEM_ATTACH:
        case XPMEM_DETACH:
        case XPMEM_MAKE_COMPLETE:
        case XPMEM_REMOVE_COMPLETE:
        case XPMEM_GET_COMPLETE:
        case XPMEM_RELEASE_COMPLETE:
        case XPMEM_ATTACH_COMPLETE: 
        case XPMEM_DETACH_COMPLETE: {
            ctrl = kmalloc(sizeof(struct pisces_xpmem_cmd_ctrl) + pfn_len, GFP_KERNEL);

            if (!ctrl) {
                return -ENOMEM;
            }

            ctrl->xpmem_cmd = cmd_ex;

            if (cmd_ex.type == XPMEM_ATTACH_COMPLETE) {
                if (pfn_len > 0) { 
                    memcpy(ctrl->pfn_list, cmd_ex.attach.pfns, cmd_ex.attach.num_pfns * sizeof(u64));
                    kfree(cmd_ex.attach.pfns);
                }
            }

            status = pisces_xpmem_cmd_ctrl(enclave, ctrl, sizeof(struct pisces_xpmem_cmd_ctrl) + pfn_len);

            if (status) {
                ret = -EFAULT;
            }
            break;
        }

        default:
            ret = -EINVAL;
            break;
    }

    kfree(ctrl);
    return ret;
}

static unsigned int
xpmem_poll(struct file              * filp,
	   struct poll_table_struct * pollp)
{
    struct pisces_enclave * enclave = (struct pisces_enclave *)filp->private_data;
    struct pisces_xpmem   * xpmem   = &(enclave->xpmem);
    unsigned int  ret = 0;
    unsigned long flags;

    if (!xpmem->initialized) {
        return -EBADF;
    }

    ret = POLLOUT | POLLWRNORM;

    poll_wait(filp, &(xpmem->user_waitq), pollp);

    spin_lock_irqsave(&(xpmem->in_lock), flags);
    {
	if (!(list_empty(&(xpmem->in_cmds)))) {
	    ret |= (POLLIN | POLLRDNORM);
	}
    }
    spin_unlock_irqrestore(&(xpmem->in_lock), flags);

    return ret;
}


static struct file_operations xpmem_fops = {
    .open       = xpmem_open,
    .release    = xpmem_release,
    .read       = xpmem_read,
    .write      = xpmem_write,
    .poll       = xpmem_poll,
};

int 
pisces_xpmem_init(struct pisces_enclave * enclave)
{
    struct pisces_xpmem       * xpmem       = &(enclave->xpmem);
    struct pisces_boot_params * boot_params = NULL;

    memset(xpmem, 0, sizeof(struct pisces_xpmem));

    boot_params = __va(enclave->bootmem_addr_pa);

    xpmem->xbuf_desc = pisces_xbuf_client_init(enclave, (uintptr_t)__va(boot_params->xpmem_buf_addr), 0, 0);

    if (!xpmem->xbuf_desc) {
        return -1;
    }

    init_waitqueue_head(&(xpmem->user_waitq));
    INIT_LIST_HEAD(&(xpmem->in_cmds));
    spin_lock_init(&(xpmem->in_lock));
    spin_lock_init(&(xpmem->state_lock));

    xpmem->fd          = -1;
    xpmem->initialized = 1;

    return 0;
}


int 
pisces_xpmem_connect(struct pisces_enclave * enclave) 
{
    struct pisces_xpmem * xpmem = &(enclave->xpmem);
    unsigned long flags         = 0;
    int connected               = 0;

    spin_lock_irqsave(&(xpmem->state_lock), flags);
    {
	if (xpmem->fd == -1) {
	    xpmem->fd = anon_inode_getfd("enclave-xpmem", &xpmem_fops, enclave, O_RDWR);
	    
	    if (xpmem->fd < 0) {
		printk(KERN_ERR "Error creating Pisces XPMEM inode\n");
	    } else {
		connected = 1;
	    }
	}   
	
    }
    spin_unlock_irqrestore(&(xpmem->state_lock), flags);

    if (connected == 0) {
	printk("Could not connect to Pisces XPMEM control channel\n");
	return -1;
    }

    return xpmem->fd;
}


/* Incoming lcall - add to list and wake up user waitq */
int 
pisces_xpmem_cmd_lcall(struct pisces_enclave   * enclave, 
		       struct pisces_xbuf_desc * xbuf_desc, 
		       struct pisces_lcall     * lcall) 
{
    struct pisces_xpmem           * xpmem            = &(enclave->xpmem);
    struct pisces_xpmem_cmd_lcall * xpmem_lcall      = (struct pisces_xpmem_cmd_lcall *)lcall;
    struct pisces_lcall_resp        lcall_resp;
    struct xpmem_cmd_ex           * xpmem_cmd        = NULL;
    struct xpmem_cmd_iter         * iter             = NULL;
    unsigned long flags = 0;

    lcall_resp.status   = 0;
    lcall_resp.data_len = 0;

    if (!xpmem->initialized) {
        printk(KERN_ERR "Cannot handle enclave XPMEM request - channel not initialized\n");

	lcall_resp.status = -1;
        goto out;
    }

    xpmem_cmd        = &(xpmem_lcall->xpmem_cmd);

    iter             = kmalloc(sizeof(struct xpmem_cmd_iter), GFP_KERNEL);

    if (!iter) {
        return -ENOMEM;
    }

    iter->cmd        = kmalloc(sizeof(struct xpmem_cmd_ex),   GFP_KERNEL);

    if (!iter->cmd) {
        kfree(iter);
        return -ENOMEM;
    }

    printk("Received %d data bytes in LCALL (xpmem_cmd_ex size: %d)\n",
	   (int)xpmem_lcall->lcall.data_len,
	   (int)sizeof(struct xpmem_cmd_ex));

    *(iter->cmd) = *(xpmem_cmd);

    if (iter->cmd->type == XPMEM_ATTACH_COMPLETE) {
        struct xpmem_cmd_attach_ex * cmd_attach  = &(xpmem_cmd->attach);
        struct xpmem_cmd_attach_ex * iter_attach = &(iter->cmd->attach);

        iter_attach->pfns = kmalloc(cmd_attach->num_pfns * sizeof(u64), GFP_KERNEL);

        if (!iter_attach->pfns) {
            kfree(iter->cmd);
            kfree(iter);
            return -ENOMEM;
        }

        memcpy(iter_attach->pfns, xpmem_lcall->pfn_list, cmd_attach->num_pfns * sizeof(u64));
    }
    
    spin_lock_irqsave(&(xpmem->in_lock), flags);
    {
	list_add_tail(&(iter->node), &(xpmem->in_cmds));
    }
    spin_unlock_irqrestore(&(xpmem->in_lock), flags);

    __asm__ __volatile__("":::"memory");
    wake_up_interruptible(&(xpmem->user_waitq));


 out:
    pisces_xbuf_complete(xbuf_desc, 
			 (u8 *)&lcall_resp,
			 sizeof(struct pisces_lcall_resp));

    return 0;

}
