/* Pisces Console / Userland interface
 * (c) 2013, Jiannan Ouyang (ouyang@cs.pitt.edu)
 * (c) 2013, Jack Lange (jacklange@cs.pitt.edu)
 */

#include <linux/fs.h>    /* device file */
#include <asm/uaccess.h>

#include "pisces_cons.h"
#include "pisces_dev.h"
#include "enclave.h"



int console_read(struct file *file, char __user *buffer,
		 size_t length, loff_t *offset) {
    extern struct pisces_enclave * enclave;
    struct pisces_cons_ringbuf * ringbuf = enclave->cons.cons_ringbuf;
    
    if (length > ringbuf->write_idx) {
	length = (ringbuf->write_idx - ringbuf->read_idx);
    }

    if (copy_to_user(buffer + *offset, ringbuf->buf + ringbuf->read_idx, length)) {
	printk(KERN_ERR "Error copying console data to user space\n");
	return -EFAULT;
    }

    ringbuf->read_idx += length;
    ringbuf->read_idx %= (64 * 1024) - 24;
    
    return length;
}



int pisces_cons_init(struct pisces_enclave * enclave, 
		     struct pisces_cons_ringbuf * ringbuf) {


    return 0;

}
