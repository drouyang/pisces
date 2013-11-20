/* 
 * Remote system call implementation 
 * (c) Jack Lange, 2013 (jacklange@cs.pitt.edu)
 */

#include <linux/types.h>
#include <linux/sched.h>
#include <linux/anon_inodes.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/kthread.h>

#include "pisces.h"
#include "pisces_boot_params.h"
#include "pisces_lcall.h"
#include "enclave.h"
#include "boot.h"
#include "pisces_cmds.h"
#include "ipi.h"
#include "util-hashtable.h"
#include "pisces_data.h"
#include "enclave_fs.h"


static ssize_t
lcall_read(struct file * filp, char __user * buffer, size_t length, loff_t * offset ) {
    struct pisces_enclave * enclave = filp->private_data;
    struct pisces_lcall * lcall = &(enclave->lcall);
    struct pisces_cmd_buf * cmd_buf = lcall->cmd_buf;
    unsigned long flags;
    int ret = 0;

    spin_lock_irqsave(&lcall->lock, flags);


    // read potential resp data

    cmd_buf->completed = 0;
    cmd_buf->active = 0;

    spin_unlock_irqrestore(&lcall->lock, flags);

    return ret;
}

// Allow lcall server to write in a raw command
static ssize_t lcall_write(struct file * filp, const char __user * buffer, size_t length, loff_t * offset) {
    //   struct pisces_enclave * enclave = filp->private_data;
    //   struct pisces_lcall * lcall = &(enclave->lcall);
    return 0;
}



// Allow high level control commands over ioctl
static long lcall_ioctl(struct file * filp, unsigned int ioctl, unsigned long arg) {

    return 0;
}


static int lcall_release(struct inode * i, struct file * filp) {
    struct pisces_enclave * enclave = filp->private_data;
    struct pisces_lcall * lcall = &(enclave->lcall);
    struct pisces_cmd_buf * cmd_buf = lcall->cmd_buf;
    unsigned long flags;

    spin_lock_irqsave(&(lcall->lock), flags);
    lcall->connected = 0; 

    if (cmd_buf->active == 1) {
    cmd_buf->resp.status = -1;
    cmd_buf->resp.data_len = 0;
    cmd_buf->completed = 1;
    }

    cmd_buf->ready = 0;
 
    spin_unlock_irqrestore(&(lcall->lock), flags);

    return 0;
}


static struct file_operations lcall_fops = {
    .write    = lcall_write,
    .read     = lcall_read,
    .unlocked_ioctl    = lcall_ioctl,
    .release  = lcall_release,
};



int pisces_lcall_connect(struct pisces_enclave * enclave) {
    struct pisces_lcall * lcall = &(enclave->lcall);
    unsigned long flags = 0;
    int acquired = 0;
    int lcall_fd = 0;

    spin_lock_irqsave(&lcall->lock, flags);
    if (lcall->connected == 0) {
        lcall->connected = 1;
        acquired = 1;
    }
    spin_unlock_irqrestore(&lcall->lock, flags);

    if (acquired == 0) {
        printk(KERN_ERR "Lcall already connected\n");
        return -1;
    }

    lcall_fd = anon_inode_getfd("enclave-lcall", &lcall_fops, enclave, O_RDWR);

    if (lcall_fd < 0) {
        printk(KERN_ERR "Error creating Lcall inode\n");
        return lcall_fd;
    }

 
    return lcall_fd;
}

static int handle_critical_lcall(struct pisces_enclave * enclave, 
                u32 lcall_id, 
                struct pisces_cmd_buf * cmd_buf) {
     
    struct pisces_resp resp;

    switch (lcall_id) {
    default:
        resp.status = -1;
        resp.data_len = 0;

        pisces_send_resp(enclave, &resp);
        return -1;
    }

    return 0;
}


static void lcall_kick(void * private_data) {
    struct pisces_enclave * enclave = private_data;
    struct pisces_lcall * lcall_state = &(enclave->lcall);
    struct pisces_cmd_buf * cmd_buf = lcall_state->cmd_buf;
    struct pisces_cmd * cmd = NULL;

    /* Make sure it's ours. NOTE: with the new staging semantics, active is not
     * true when the IPI is sent, but rather staging is true */
    //if (cmd_buf->active != 1 || cmd_buf->in_progress != 0)
    if (cmd_buf->staging != 1) {
        return;
    }

    printk("LCALL is kicked (cmd: %llu)\n", cmd_buf->cmd.cmd);

    if (pisces_recv_cmd(enclave, &(lcall_state->cmd)) != 0) {
        printk(KERN_ERR "Error receiving Longcall command\n");
        return;
    }

    cmd = lcall_state->cmd;

    if ((cmd->cmd >= CRIT_LCALL_START) && 
        (cmd->cmd < KERN_LCALL_START)) {
        // Call the handler from the IRQ handler
        handle_critical_lcall(enclave, cmd->cmd, cmd_buf);
    } else if ((cmd->cmd >= KERN_LCALL_START) && 
           (cmd->cmd < USER_LCALL_START)) {

        printk("Waking up kernel thread\n");
        wake_up_interruptible(&(lcall_state->kern_waitq));
    } else {
        wake_up_interruptible(&(lcall_state->user_waitq));
    }

    return;
}

static int lcall_kern_thread(void * arg) {
    struct pisces_enclave * enclave = arg;
    struct pisces_lcall * lcall_state = &(enclave->lcall);
    struct pisces_cmd_buf * cmd_buf = lcall_state->cmd_buf;
    //struct pisces_cmd * cmd = &(cmd_buf->cmd);
    //struct pisces_resp * resp = &(cmd_buf->resp);
    struct pisces_cmd * cmd = NULL;
    struct pisces_resp resp;
 

    while (1) {

	//  printk("LCALL Kernel thread going to sleep on cmd buf\n");

	wait_event_interruptible(lcall_state->kern_waitq, 
         (cmd_buf->active == 1));

    
    
	printk("kernel thread is awake\n");

    cmd = lcall_state->cmd;
    
	switch (cmd->cmd) {
	    case PISCES_LCALL_VFS_READ:
		enclave_vfs_read_lcall(enclave, cmd);
		break;
	    case PISCES_LCALL_VFS_WRITE:
		enclave_vfs_write_lcall(enclave, cmd);
		break;
	    case PISCES_LCALL_VFS_OPEN: 
		enclave_vfs_open_lcall(enclave, cmd);
		break;
	    case PISCES_LCALL_VFS_CLOSE:
		enclave_vfs_close_lcall(enclave, cmd);
		break;
	    case PISCES_LCALL_VFS_SIZE:
		enclave_vfs_size_lcall(enclave, cmd);
		break;

        case PISCES_LCALL_XPMEM_ATTACH:
            pisces_portals_xpmem_attach(enclave, cmd);
            break;

	    case PISCES_LCALL_VFS_READDIR:
	    default:
		printk(KERN_ERR "Enclave requested unimplemented LCALL %llu\n", cmd->cmd);
		resp.status = -1;
		resp.data_len = 0;
        pisces_send_resp(enclave, &resp);
		break;
	}

    }

    return 0;
}


int
pisces_lcall_init( struct pisces_enclave * enclave) {
    struct pisces_lcall * lcall_state = &(enclave->lcall);
    struct pisces_boot_params * boot_params = NULL;
 

    init_waitqueue_head(&lcall_state->user_waitq);
    init_waitqueue_head(&lcall_state->kern_waitq);
    spin_lock_init(&lcall_state->lock);
    lcall_state->connected = 0;

    boot_params = __va(enclave->bootmem_addr_pa);
    lcall_state->cmd_buf = (struct pisces_cmd_buf *)__va(boot_params->longcall_buf_addr);

    memset(lcall_state->cmd_buf, 0, sizeof(struct pisces_cmd_buf));

    {
    char thrd_name[32];
    memset(thrd_name, 0, 32);

    snprintf(thrd_name, 32, "enclave%d-lcalld", enclave->id);
    
    lcall_state->kern_thread = kthread_create(lcall_kern_thread, enclave, thrd_name);
    wake_up_process(lcall_state->kern_thread);
    }



    if (pisces_register_ipi_callback(lcall_kick, enclave) != 0) {
    printk(KERN_ERR "Error registering lcall IPI callback for enclave %d\n", enclave->id);
    return -1;
    }

    // Notify the enclave we are ready to accept commands
    lcall_state->cmd_buf->ready = 1;
    lcall_state->cmd_buf->host_vector = STUPID_LINUX_IRQ;
    lcall_state->cmd_buf->host_apic = 0; // this should be dynamic, but we know that 0 will always be there



    return 0;
}
