/* Pisces Enclave 
 * (c) 2013, Jack Lange, (jacklange@cs.pitt.edu)
 * (c) 2013, Jiannan Ouyang, (ouyang@cs.pitt.edu)
 */

#ifndef __PISCES_CONS_H__
#define __PISCES_CONS_H__

#include <linux/types.h>
#include "pisces_lock.h"

struct pisces_enclave;

// Embedded ringbuffer that maps into a 64KB chunk of memory
struct pisces_cons_ringbuf {
    struct pisces_spinlock lock;
    u64 read_idx;
    u64 write_idx;
    u64 cur_len;
    u8 buf[(64 * 1024) - 32];
} __attribute__((packed));




struct pisces_cons {
    
    struct pisces_cons_ringbuf * cons_ringbuf;

} __attribute__((packed));



int console_read(struct file *file, char __user *buffer,
		 size_t length, loff_t *offset);

int pisces_cons_init(struct pisces_enclave * enclave, 
		     struct pisces_cons_ringbuf * ringbuf);




#endif
