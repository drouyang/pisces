/* Pisces Enclave 
 * (c) 2013, Jack Lange, (jacklange@cs.pitt.edu)
 * (c) 2013, Jiannan Ouyang, (ouyang@cs.pitt.edu)
 */

#ifndef __ENCLAVE_CONS_H__
#define __ENCLAVE_CONS_H__

#include <linux/types.h>
#include <linux/fs.h>
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
    int connected;
    spinlock_t  lock;
} __attribute__((packed));



int pisces_cons_init(struct pisces_enclave * enclave, 
                     struct pisces_cons_ringbuf * ringbuf);

int pisces_enclave_get_cons(struct pisces_enclave * enclave);




#endif
