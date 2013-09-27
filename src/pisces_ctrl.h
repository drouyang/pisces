#ifndef _PISCES_CTRL_H_
#define _PISCES_CTRL_H_

#include <linux/wait.h>
#include "pisces.h"
#include "pisces_ringbuf.h"

struct pisces_enclave;

struct pisces_ctrl {
    struct pisces_early_ringbuf * buf;
    wait_queue_head_t waitq;
    spinlock_t lock;
} __attribute__((packed));


int pisces_ctrl_init(struct pisces_ctrl * ctrl, 
        struct pisces_early_ringbuf * ctrl_ringbuf);

int pisces_ctrl_add_cpu(struct pisces_enclave * enclave, int apicid);
int pisces_ctrl_send(struct pisces_enclave * enclave,
        struct pisces_ctrl_cmd * cmd);
int pisces_ctrl_recv(struct pisces_enclave * enclave,
        struct pisces_ctrl_cmd * cmd);

#endif
