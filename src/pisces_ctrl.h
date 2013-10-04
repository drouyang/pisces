#ifndef _PISCES_CTRL_H_
#define _PISCES_CTRL_H_

#include <linux/wait.h>
#include <linux/types.h>
#include "pisces.h"

struct pisces_enclave;

struct ctrl_ioctl_mem_add {
	u64 start_addr;
	u64 size;
};


struct pisces_ctrl {
    int connected;
    wait_queue_head_t waitq;
    spinlock_t lock;

    struct ctrl_cmd_buf * cmd_buf;
} __attribute__((packed));

int pisces_ctrl_init(struct pisces_enclave * enclave);

int pisces_ctrl_connect(struct pisces_enclave * enclave);

#endif
