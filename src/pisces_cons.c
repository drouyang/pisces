/* Pisces Console / Userland interface
 * (c) 2013, Jiannan Ouyang (ouyang@cs.pitt.edu)
 * (c) 2013, Jack Lange (jacklange@cs.pitt.edu)
 */

#include <linux/fs.h>    /* device file */
#include <asm/uaccess.h>

#include "pisces_cons.h"
#include "enclave.h"




int console_read(struct file *file, char __user *buffer,
		 size_t length, loff_t *offset) {
    struct pisces_enclave * enclave = (struct pisces_enclave *)file->private_data;
    struct pisces_cons_ringbuf * ringbuf = enclave->cons.cons_ringbuf;
    size_t left_to_read = 0;
    u64 read_len = 0;

    pisces_spin_lock(&(ringbuf->lock));

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
	    
	    if (copy_to_user(buffer + *offset, ringbuf->buf + ringbuf->read_idx, read_len)) {
		printk(KERN_ERR "Error copying console data to user space\n");
		pisces_spin_unlock(&(ringbuf->lock));
		return -EFAULT;
	    }
	    
	    *offset += read_len;
	    ringbuf->read_idx += read_len;
	    ringbuf->read_idx %= sizeof(ringbuf->buf);
	    ringbuf->cur_len -= read_len;
	    left_to_read -= read_len;
	}
    } 
	
    if (left_to_read > 0) {
	
	read_len = ringbuf->write_idx - ringbuf->read_idx;

	if (read_len > left_to_read) {
	    read_len = left_to_read;
	}

	// read from read_idx to write_idx
	if (copy_to_user(buffer + *offset, ringbuf->buf + ringbuf->read_idx, read_len)) {
	    printk(KERN_ERR "Error copying console data to user space\n");
	    pisces_spin_unlock(&(ringbuf->lock));
	    return -EFAULT;
	}
	
	*offset += read_len;
	ringbuf->read_idx += read_len;
	ringbuf->read_idx %= sizeof(ringbuf->buf);
	ringbuf->cur_len -= read_len;
	left_to_read -= read_len;
	
    }

    pisces_spin_unlock(&(ringbuf->lock));
    

    return length;
}



int pisces_cons_init(struct pisces_enclave * enclave, 
		     struct pisces_cons_ringbuf * ringbuf) {

    enclave->cons.cons_ringbuf = ringbuf;
    pisces_lock_init(&(ringbuf->lock));

    return 0;
}
