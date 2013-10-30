#ifndef _PISCES_CTRL_H_
#define _PISCES_CTRL_H_

#include <linux/wait.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <asm/irq_vectors.h>

#include "pisces.h"
#include "pisces_cmds.h"

struct pisces_enclave;

struct pisces_ctrl {
    int connected;
    wait_queue_head_t waitq;
    spinlock_t lock;

    struct pisces_cmd_buf * cmd_buf;
} __attribute__((packed));

int pisces_ctrl_init(struct pisces_enclave * enclave);

int pisces_ctrl_connect(struct pisces_enclave * enclave);

#endif
