/* 
 * VM Console 
 * (c) 2013, Jack Lange <jacklange@cs.pitt.edu>
 */

#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/poll.h>
#include <linux/anon_inodes.h>
#include <linux/file.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/spinlock.h>

#include "util-queue.h"
#include "ipi.h"
#include "pisces_lock.h"
#include "enclave.h"
#include "ctrl_cmds.h"
#include "pisces_xbuf.h"
#include "enclave_ctrl.h"

typedef enum { CONSOLE_CURS_SET   = 1,
	       CONSOLE_CHAR_SET   = 2,
	       CONSOLE_SCROLL     = 3,
	       CONSOLE_UPDATE     = 4,
               CONSOLE_RESOLUTION = 5} console_op_t;




struct cursor_msg {
    int x;
    int y;
} __attribute__((packed));

struct character_msg {
    int  x;
    int  y;
    char c;
    unsigned char style;
} __attribute__((packed));

struct scroll_msg {
    int lines;
} __attribute__((packed));


struct resolution_msg {
    int cols;
    int rows;
} __attribute__((packed));

struct cons_msg {
    unsigned char op;
    union {
	struct cursor_msg     cursor;
	struct character_msg  character;
	struct scroll_msg     scroll;
	struct resolution_msg resolution;
    };
} __attribute__((packed)); 



struct cons_ring_buf {
    struct pisces_spinlock lock;
    u16 read_idx;
    u16 write_idx;
    u16 cur_entries;
    u16 total_entries;
    u16 kick_ipi_vec;
    u16 kick_apic;

    struct cons_msg msgs[0];
} __attribute__((packed));

struct palacios_console {
    struct pisces_enclave * enclave;
    u32 vm_id;

    spinlock_t irq_lock;
    wait_queue_head_t intr_queue;

    struct cons_ring_buf * ring_buf;

};


static void cons_kick(void * arg) {
    struct palacios_console * cons = arg;
    u32 entries = 0;

    entries = cons->ring_buf->cur_entries;

    if (entries > 0) {
	wake_up_interruptible(&(cons->intr_queue));
    }

    return;
}


static int 
cons_dequeue(struct cons_ring_buf * ringbuf, 
	     struct cons_msg      * msg) 
{
    int ret = -1;

    pisces_spin_lock(&(ringbuf->lock));
    {
	if (ringbuf->cur_entries > 0) {	    
	    memcpy(msg, &(ringbuf->msgs[ringbuf->read_idx]), sizeof(struct cons_msg));
	    
	    __asm__ __volatile__ ("lock decw %1;"
				  : "+m"(ringbuf->cur_entries)
				  :
				  : "memory");
	    
	    ringbuf->read_idx += 1;
	    ringbuf->read_idx %= ringbuf->total_entries;

	    ret = 0;
	}
    }
    pisces_spin_unlock(&(ringbuf->lock));
    
    return ret;
}





static ssize_t 
console_read(struct file * filp, 
	     char __user * buf, 
	     size_t        size, 
	     loff_t      * offset) 
{
    struct palacios_console * cons = filp->private_data;
    struct cons_msg msg;
    int ret = 0;

    memset(&msg, 0, sizeof(struct cons_msg));

    if (size != sizeof(struct cons_msg)) {
	printk(KERN_ERR "Invalid Read operation size: %lu\n", size);
	return -EFAULT;
    }

    wait_event_interruptible(cons->intr_queue, (cons->ring_buf->cur_entries >= 1));

    
    ret = cons_dequeue(cons->ring_buf, &msg);
    
    
    if (ret == -1) {
	printk(KERN_ERR "ERROR: Null console message\n");
	return -EFAULT;
    }
    
    if (copy_to_user(buf, &msg, size)) {
	printk(KERN_ERR "Read Fault\n");
	return -EFAULT;
    }
 

    return size;
}


static ssize_t 
console_write(struct file       * filp,
	      const char __user * buf, 
	      size_t              size, 
	      loff_t            * offset) 
{
    struct palacios_console    * cons      = filp->private_data;
    struct pisces_xbuf_desc    * xbuf_desc = cons->enclave->ctrl.xbuf_desc;
    struct cmd_vm_cons_keycode   cmd;
    int i = 0;

    memset(&cmd, 0, sizeof(struct cmd_vm_cons_keycode));


    cmd.hdr.cmd      = ENCLAVE_CMD_VM_CONS_KEYCODE;
    cmd.hdr.data_len = (sizeof(struct cmd_vm_cons_keycode) - sizeof(struct pisces_cmd));
    cmd.vm_id        = cons->vm_id;
  
    for (i = 0; i < size; i++) {
	if (copy_from_user(&(cmd.scan_code), buf + i, 1)) {
	    printk(KERN_ERR "Console Write fault\n");
	    return -EFAULT;
	}
	
	 //printk("Sending Scan_Code %x\n", cmd.scan_code);

	 pisces_xbuf_send(xbuf_desc, (u8 *)&cmd, sizeof(struct cmd_vm_cons_keycode));
    }
    
    return size;
}

static unsigned int 
console_poll(struct file              * filp,
	     struct poll_table_struct * poll_tb) 
{
    struct palacios_console * cons = filp->private_data;
    unsigned int              mask = POLLIN | POLLRDNORM;
    u32 entries = 0;

    //    printk(KERN_DEBUG "Console=%p (guest=%s)\n", cons, cons->guest->name);


    poll_wait(filp, &(cons->intr_queue), poll_tb);

    pisces_spin_lock(&(cons->ring_buf->lock));
    {
	entries = cons->ring_buf->cur_entries;
    }
    pisces_spin_unlock(&(cons->ring_buf->lock));

    if (entries > 0) {
	//	printk(KERN_DEBUG "Returning from POLL\n");
	return mask;
    }
    
    return 0;
}


static int 
console_release(struct inode * i, 
		struct file  * filp) 
{
    struct palacios_console * cons      = filp->private_data;
    struct pisces_enclave   * enclave   = cons->enclave;
    struct pisces_xbuf_desc * xbuf_desc = enclave->ctrl.xbuf_desc;
    struct cmd_vm_ctrl        cmd;
    int ret = 0;

    printk(KERN_DEBUG "Releasing the Console File desc\n");

    memset(&cmd, 0, sizeof(struct cmd_vm_ctrl));
    
    cmd.hdr.cmd      = ENCLAVE_CMD_VM_CONS_DISCONNECT;
    cmd.hdr.data_len = (sizeof(struct cmd_vm_ctrl) - sizeof(struct pisces_cmd));
    cmd.vm_id        = cons->vm_id;
 
    // disconnect console
    ret = pisces_xbuf_send(xbuf_desc, (u8 *)&cmd, sizeof(struct cmd_vm_ctrl));

    pisces_remove_ipi_callback(cons_kick, cons);

    if (ret != 0) {
	printk(KERN_ERR "Error sending disconnect message to VM %d on enclave (%d)\n", 
	       cons->vm_id, enclave->id);
    }


    kfree(cons);
    

    return 0;
}


static struct file_operations cons_fops = {
    .read     = console_read,
    .write    = console_write,
    .poll     = console_poll,
    .release  = console_release,
};





int 
v3_console_connect(struct pisces_enclave * enclave,
		   u32                     vm_id, 
		   uintptr_t               cons_buf_pa) 
{
    struct palacios_console * cons = NULL;
    int cons_fd = 0;

    cons = kmalloc(sizeof(struct palacios_console), GFP_KERNEL);
    
    if (IS_ERR(cons)) {
	printk(KERN_ERR "Error allocating pisces VM  console state\n");
	return -1;
    }

    cons->enclave                = enclave;
    cons->vm_id                  = vm_id;
    cons->ring_buf               = __va(cons_buf_pa);
    cons->ring_buf->kick_apic    = apic->cpu_present_to_apicid(0);

    init_waitqueue_head(&(cons->intr_queue));
    spin_lock_init(&(cons->irq_lock));

    pisces_register_ipi_callback(cons_kick, cons);

    __asm__ __volatile__ ("" ::: "memory");

    cons->ring_buf->kick_ipi_vec = PISCES_LCALL_VECTOR;



    cons_fd = anon_inode_getfd("v3-cons", &cons_fops, cons, O_RDWR);

    if (cons_fd < 0) {
	printk(KERN_ERR "Error creating console inode\n");
	return cons_fd;
    }


    printk("V3 Console connected\n");

    return cons_fd;
}

