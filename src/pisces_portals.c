/* 
 * Portals setup communication 
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


static ssize_t
portals_read(struct file * filp, char __user * buffer, size_t length, loff_t * offset ) {
    struct pisces_enclave * enclave = filp->private_data;
    struct pisces_lcall * lcall_state = &(enclave->lcall);
    struct pisces_cmd_buf * cmd_buf = lcall_state->cmd_buf;
    struct pisces_resp * resp = &(cmd_buf->resp);

    struct portals_ppe_cmd * ppe_cmd = (struct portals_ppe_cmd *)&(cmd_buf->cmd);
    ssize_t ret = 0, copy_len = 0;

    /* Put the process to sleep unless data is already written by the enclave */
    wait_event_interruptible(lcall_state->user_waitq,
                ((cmd_buf->active == 1) && 
                 (cmd_buf->in_progress == 0)));

    /* Set up the userspace structure */
    if (length > ppe_cmd->cmd.data_len) {
        copy_len = ppe_cmd->cmd.data_len;
    } else {
        copy_len = length;
    }

    if (copy_to_user(buffer, ppe_cmd->msg, copy_len)) {
        printk(KERN_ERR "Error copying Portals command to userspace\n");
        resp->status = -1;
        resp->data_len = 0;
        ret = -1;
        cmd_buf->completed = 1;
    } else {
        ret = copy_len;
    }

    return ret;
}

static ssize_t 
portals_write(struct file * filp, const char __user * buffer, size_t length, loff_t * offset) {
    struct pisces_enclave * enclave = filp->private_data;
    struct pisces_lcall * lcall_state = &(enclave->lcall);
    struct pisces_cmd_buf * cmd_buf = lcall_state->cmd_buf;
    struct pisces_resp * resp = &(cmd_buf->resp); 

    ssize_t ret;

    /* Copy data to portals buffer */
    if (copy_from_user(resp->data, buffer, length)) {
        printk(KERN_ERR "Error copying Portals response from userspace\n");
        resp->status = -1;
        resp->data_len = 0;
        ret = -1;
    } else {
        /* Signal that the result is written */
        resp->status = length;
        resp->data_len = length;
        ret = length;
    }

    cmd_buf->completed = 1;
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

static struct file_operations portals_fops = {
    .write    = portals_write,
    .read     = portals_read,
    .release  = portals_release,
};

int pisces_portals_init(struct pisces_enclave * enclave) {
    struct pisces_portals * portals = &(enclave->portals);

    spin_lock_init(&portals->lock);
    portals->connected = 0;

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
