/* Pisces Ringbuffer 
 * Basic communication channel between enclaves
 * (c) 2013, Jack Lange (jacklange@cs.pitt.edu)
 * (c) 2013, Jiannan Ouyang (ouyang@cs.pitt.edu)
 */



#ifndef __PISCES_RINGBUF_H__
#define __PISCES_RINGBUF_H__

#include <linux/types.h>
#include "pisces_lock.h"

#define EARLY_RINGBUF_SIZE (1024 * 8)

/* this is provides a fully internalized ringbuffer implementation
 * This is meant for early initialization 
 *     where we need static sized data structures
 */
struct pisces_early_ringbuf {
    u64 read_idx;
    u64 write_idx;
    u64 size;
    u64 cur_len;

    struct pisces_spinlock lock;
  
    u8 buf[EARLY_RINGBUF_SIZE];

} __attribute__((packed));


int pisces_early_ringbuf_init(struct pisces_early_ringbuf * ringbuf);

int pisces_early_ringbuf_write(struct pisces_early_ringbuf * ringbuf,
			       u8 * data, u64 size);
int pisces_early_ringbuf_read(struct pisces_early_ringbuf * ringbuf, 
			      u8 * data, u64 size);



#endif
