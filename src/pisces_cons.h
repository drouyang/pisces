#ifndef __PISCES_CONS_H__
#define __PISCES_CONS_H__



#include "pisces_lock.h"
#include "pisces_ringbuf.h"




// Embedded ringbuffer that maps into a 64KB chunk of memory
struct pisces_cons_ringbuf {
    u64 lock;
    u64 read_idx;
    u64 write_idx;
    u8 buf[(64 * 1024) - 24];
} __attribute__((packed));




struct pisces_cons {
    // in buffer 1K
    //struct pisces_spinlock lock_in;
    //char in[PISCES_CONSOLE_SIZE_IN];
    //u64 in_cons, in_prod;
    
    struct pisces_cons_ringbuf * cons_ringbuf;

} __attribute__((packed));


#endif
