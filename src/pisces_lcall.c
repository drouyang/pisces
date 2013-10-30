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

#define DEFAULT_LCALL  0 // forward straight to user space, without any handling in the kernel
#define CRITICAL_LCALL 1 // handle immediately in IPI handler
#define KERNEL_LCALL   2 // handled by internal kernel implementation

struct lcall {
    u32 lcall_id;
    u8 type;
    int (*handler)(u32 lcall_id, void * priv_data, struct pisces_cmd_buf * cmd_buf);
    void * priv_data;
};


static struct hashtable * lcall_table = NULL;

static u32 lcall_hash_fn(uintptr_t key) {
    return hash_long(key);
}

static int lcall_eq_fn(uintptr_t key1, uintptr_t key2) {
    return (key1 == key2);
}


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



int pisces_register_lcall_handler(u32 lcall_id, u8 type, 
				  int (*handler)(u32 lcall_id, 
						 void * priv_data, 
						 struct pisces_cmd_buf * cmd_buf), 
				  void * priv_data) {
    struct lcall * new_lcall = NULL;

    if (htable_search(lcall_table, lcall_id)) {
	printk(KERN_ERR "lcall %d is already registered\n", lcall_id);
	return -1;
    }

    new_lcall = kmalloc(sizeof(struct lcall), GFP_KERNEL);

    if (IS_ERR(new_lcall)) {
	printk(KERN_ERR "Could not allocate new lcall\n");
	return -1;
    }

    new_lcall->lcall_id = lcall_id;
    new_lcall->type = type;
    new_lcall->handler = handler;
    new_lcall->priv_data = priv_data;

    if (htable_insert(lcall_table, lcall_id, (uintptr_t)new_lcall) == 0) {
	printk(KERN_ERR "Could not insert lcall (%d) into lcall table\n", lcall_id);
	return -1;
    }
    
    return 0;
}


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

static void lcall_kick(void * private_data) {
    struct pisces_enclave * enclave = private_data;
    struct pisces_lcall * lcall_state = &(enclave->lcall);
    struct pisces_cmd_buf * cmd_buf = lcall_state->cmd_buf;
    struct pisces_cmd * cmd = &(cmd_buf->cmd);
    struct lcall * lcall = NULL;

    printk("LCALL is kicked\n");


    lcall = (struct lcall *)htable_search(lcall_table, cmd->cmd);

    if (lcall == NULL) {
	struct pisces_resp * resp = &(cmd_buf->resp);

	printk(KERN_ERR "Could not find handler for lcall %llu\n", cmd->cmd);

	resp->status = -1;
	resp->data_len = 0;

	cmd_buf->completed = 1;

	return;
    }

    if (lcall->type == CRITICAL_LCALL) {
	// Call the handler from the IRQ handler
	cmd_buf->in_progress = 1;

	lcall->handler(lcall->lcall_id, lcall->priv_data, cmd_buf);

	cmd_buf->completed = 1;
    } else if (lcall->type == KERNEL_LCALL) {
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
    struct pisces_cmd * cmd = &(cmd_buf->cmd);
    struct lcall * lcall = NULL;

    while (wait_event_interruptible(lcall_state->kern_waitq, 
				    ((cmd_buf->active == 1) && 
				     (cmd_buf->in_progress == 0)))) {

	lcall = (struct lcall *)htable_search(lcall_table, cmd->cmd);

	if (lcall == NULL) {
	    struct pisces_resp * resp = &(cmd_buf->resp);
	    
	    printk(KERN_ERR "Could not find handler for lcall %llu\n", cmd->cmd);
	    
	    resp->status = -1;
	    resp->data_len = 0;
	    
	    cmd_buf->completed = 1;
	    
	    continue;
	}

	cmd_buf->in_progress = 1;
	
	lcall->handler(lcall->lcall_id, lcall->priv_data, cmd_buf);

	cmd_buf->completed = 1;

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


    lcall_table = create_htable(0, lcall_hash_fn, lcall_eq_fn);

    if (IS_ERR(lcall_table)) {
	printk(KERN_ERR "Failed to allocate lcall hash table\n");
	return -1;
    }



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

