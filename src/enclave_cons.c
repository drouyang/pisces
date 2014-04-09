/* Pisces Console / Userland interface
 * (c) 2013, Jiannan Ouyang (ouyang@cs.pitt.edu)
 * (c) 2013, Jack Lange (jacklange@cs.pitt.edu)
 */

#include <linux/fs.h>    /* device file */
#include <linux/anon_inodes.h>
#include <asm/uaccess.h>

#include "enclave.h"
#include "enclave_cons.h"




static ssize_t 
console_read(struct file  * file, 
	     char __user  * buffer,
	     size_t         length, 
	     loff_t       * offset)
{
    struct pisces_enclave      * enclave = (struct pisces_enclave *)file->private_data;
    struct pisces_cons_ringbuf * ringbuf = enclave->cons.cons_ringbuf;
    size_t left_to_read = 0;
    u64    read_len     = 0;
    
    pisces_spin_lock(&(ringbuf->lock));
    {
/*     printk("console len=%llu, read_idx=%llu, write_idx=%llu\n",  */
/* 	   ringbuf->cur_len, ringbuf->read_idx, ringbuf->write_idx); */
	
	if (length > ringbuf->cur_len) {
	    length = ringbuf->cur_len;
	}
	
	left_to_read = length;
	
	if (left_to_read > 0) {
	    if (ringbuf->read_idx >= ringbuf->write_idx) {
		// first we need to read from read_idx to end of buf
		
		read_len = sizeof(ringbuf->buf) - ringbuf->read_idx;
		
		if (read_len > left_to_read) {
		    read_len = left_to_read;
		}
		
		if (copy_to_user(buffer, ringbuf->buf + ringbuf->read_idx, read_len)) {
		    printk(KERN_ERR "Error copying console data to user space\n");

		    pisces_spin_unlock(&(ringbuf->lock));

		    return -EFAULT;
		}
		
		*offset            += read_len;
		ringbuf->read_idx  += read_len;
		ringbuf->read_idx  %= sizeof(ringbuf->buf);
		ringbuf->cur_len   -= read_len;
		left_to_read       -= read_len;
	    }
	} 
	
	if (left_to_read > 0) {
	    
	    read_len = ringbuf->write_idx - ringbuf->read_idx;
	    
	    if (read_len > left_to_read) {
		read_len = left_to_read;
	    }
	    
	    // read from read_idx to write_idx
	    if (copy_to_user(buffer, ringbuf->buf + ringbuf->read_idx, read_len)) {
		printk(KERN_ERR "Error copying console data to user space\n");
		
		pisces_spin_unlock(&(ringbuf->lock));
		
		return -EFAULT;
	    }
	    
	    *offset            += read_len;
	    ringbuf->read_idx  += read_len;
	    ringbuf->read_idx  %= sizeof(ringbuf->buf);
	    ringbuf->cur_len   -= read_len;
	    left_to_read       -= read_len;
	    
	}
    }
    pisces_spin_unlock(&(ringbuf->lock));
    
    /*    printk("Read %lu bytes\n", length); */
    
    return length;
}



int 
pisces_cons_init(struct pisces_enclave      * enclave, 
		 struct pisces_cons_ringbuf * ringbuf) 
{
    struct pisces_cons * cons = &enclave->cons;

    cons->cons_ringbuf = ringbuf;
    cons->connected    = 0;

    pisces_lock_init(&(ringbuf->lock));
    spin_lock_init(&cons->lock);

    return 0;
}


static int 
console_release(struct inode * i, 
		struct file  * filp) 
{
    struct pisces_enclave * enclave = filp->private_data;
    struct pisces_cons    * cons    = &enclave->cons;
    unsigned long flags;
    
    spin_lock_irqsave(&(cons->lock), flags);
    {
	cons->connected = 0;
    }
    spin_unlock_irqrestore(&(cons->lock), flags);
    
    return 0;
}


static struct file_operations cons_fops = {
    .read     = console_read,
    .release  = console_release
};



int 
pisces_cons_connect(struct pisces_enclave * enclave)
{
    struct pisces_cons * cons  = &enclave->cons;
    unsigned long        flags = 0;

    int cons_fd  = 0;
    int acquired = 0;

    if (enclave->state != ENCLAVE_RUNNING) {
        printk(KERN_WARNING "Enclave is not running\n");
        return -1;
    }

    spin_lock_irqsave(&(cons->lock), flags);
    {
	if (cons->connected == 0) {
	    cons->connected = 1;
	    acquired        = 1;
	}
    }
    spin_unlock_irqrestore(&(cons->lock), flags);

    if (acquired == 0) {
        printk(KERN_ERR "Console already connected\n");
        return -1;
    }

    cons_fd = anon_inode_getfd("enclave-cons", &cons_fops, enclave, O_RDWR);

    if (cons_fd < 0) {
        printk(KERN_ERR "Error creating console inode\n");
        return cons_fd;
    }

    return cons_fd;
}
