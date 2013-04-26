#ifndef __PISCES_CONS_H__
#define __PISCES_CONS_H__



#include "pisces_lock.h"

// in out buffer as a circular queue
// prod is queue head, point to an available slot
// cons is queue tail
// cons == prod-1 => queue is empty
// prod == cons-1 => queue is full
#define PISCES_CONSOLE_SIZE_OUT (1024 * 6)
#define PISCES_CONSOLE_SIZE_IN 1024

struct pisces_cons {
    // in buffer 1K
    //struct pisces_spinlock lock_in;
    //char in[PISCES_CONSOLE_SIZE_IN];
    //u64 in_cons, in_prod;
    
    // out buffer 2K
    struct pisces_spinlock output_lock;
    char out[PISCES_CONSOLE_SIZE_OUT];
    u64 out_cons;
    u64 out_prod;


    char console_buffer[1024 * 50];
    u64 console_idx ;

} __attribute__((packed));


#endif
