/* 
 * Remote system call implementation 
 * (c) Jack Lange, 2013 (jacklange@cs.pitt.edu)
 */

#ifndef __PISCES_LCALL_H__
#define __PISCES_LCALL_H__

#include <linux/wait.h>
#include <linux/types.h>
#include "pisces.h"

#define CRIT_LCALL_START 0
#define KERN_LCALL_START 10000
#define USER_LCALL_START 20000

/* CRITICAL/ATOMIC LCALLS (0 - 9999) */


/* KERNEL LCALLS (10000 - 19999) */
#define PISCES_LCALL_VFS_READ      (KERN_LCALL_START + 0)
#define PISCES_LCALL_VFS_WRITE     (KERN_LCALL_START + 1)
#define PISCES_LCALL_VFS_OPEN      (KERN_LCALL_START + 2)
#define PISCES_LCALL_VFS_CLOSE     (KERN_LCALL_START + 3)
#define PISCES_LCALL_VFS_SIZE      (KERN_LCALL_START + 4)
#define PISCES_LCALL_VFS_MKDIR     (KERN_LCALL_START + 5)
#define PISCES_LCALL_VFS_READDIR   (KERN_LCALL_START + 6)

/* USER LCALLS (20000 - 29999) */


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


#endif
