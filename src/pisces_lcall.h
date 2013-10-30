/* 
 * Remote system call implementation 
 * (c) Jack Lange, 2013 (jacklange@cs.pitt.edu)
 */

#ifndef __PISCES_LCALL_H__
#define __PISCES_LCALL_H__

#include <linux/wait.h>
#include <linux/types.h>
#include "pisces.h"


struct lcall;

struct pisces_lcall {
    int connected;
    wait_queue_head_t user_waitq;
    spinlock_t lock;

    wait_queue_head_t kern_waitq;
    struct task_struct * kern_thread;

    struct pisces_cmd_buf * cmd_buf;
};


struct pisces_enclave;

int pisces_lcall_init(struct pisces_enclave * enclave);
int pisces_lcall_connect(struct pisces_enclave * enclave);


int pisces_register_lcall_handler(u32 lcall_id, u8 type, 
				  int (*handler)(u32 lcall_id, 
						 void * priv_data, 
						 struct pisces_cmd_buf * cmd_buf), 
				  void * priv_data);

#endif
