#ifndef _PISCES_CTRL_H_
#define _PISCES_CTRL_H_

#include "pisces_ringbuf.h"

struct pisces_enclave;

#define PISCES_CTRL_ADD_CPU  1
#define PISCES_CTRL_BOOT_CPU  1
#define PISCES_CTRL_ADD_MEM  3

struct pisces_ctrl {
    struct pisces_early_ringbuf * ctrl_ringbuf;
} __attribute__((packed));

struct pisces_ctrl_cmd {
    u64 cmd;
    u64 arg1;
    u64 arg2;
    u64 arg3;
} __attribute__((packed));

int pisces_ctrl_init(struct pisces_enclave * enclave, 
        struct pisces_early_ringbuf * ctrl_ringbuf);

void pisces_ctrl_add_cpu(struct pisces_enclave * enclave, int apicid);

#endif
