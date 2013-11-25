#ifndef _PISCES_CTRL_H_
#define _PISCES_CTRL_H_

#include <linux/wait.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <asm/irq_vectors.h>

#include "pisces.h"
#include "pisces_cmds.h"



#define CRIT_CTRL_START 0
#define KERN_CTRL_START 10000
#define USER_CTRL_START 20000

/* CRITICAL/ATOMIC CTRL COMMANDS */


/* KERNEL CTRL COMMANDS */
#define ENCLAVE_CMD_XPMEM_ATTACH    (KERN_CTRL_START + 200)


/* USER CTRL COMMANDS */
#define ENCLAVE_CMD_ADD_CPU         (USER_CTRL_START + 0)
#define ENCLAVE_CMD_ADD_MEM         (USER_CTRL_START + 1)
#define ENCLAVE_CMD_TEST_LCALL      (USER_CTRL_START + 2)
#define ENCLAVE_CMD_CREATE_VM       (USER_CTRL_START + 3)
#define ENCLAVE_CMD_LAUNCH_VM       (USER_CTRL_START + 4)




struct pisces_enclave;

struct pisces_ctrl {
    int connected;
    wait_queue_head_t waitq;
    spinlock_t lock;

    int active; /* Is the control channel active? Separate from cmd_buf->active */
    struct pisces_cmd_buf * cmd_buf;
} __attribute__((packed));

int pisces_ctrl_send_cmd(struct pisces_enclave * enclave, struct pisces_cmd * cmd,
                struct pisces_resp ** resp_p);
int pisces_ctrl_init(struct pisces_enclave * enclave);
int pisces_ctrl_connect(struct pisces_enclave * enclave);

#endif
