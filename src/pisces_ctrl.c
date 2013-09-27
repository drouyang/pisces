#include <linux/sched.h>
#include "pisces.h"
#include "pisces_ctrl.h"
#include "enclave.h"
#include "enclave_xcall.h"

int
pisces_ctrl_init(
        struct pisces_ctrl * ctrl,
        struct pisces_early_ringbuf * ringbuf) {

    ctrl->buf = ringbuf;
    init_waitqueue_head(&ctrl->waitq);
    spin_lock_init(&ctrl->lock);

    return 0;
}


#if 0
int 
ctrl_send(struct pisces_enclave * enclave, 
        struct pisces_ctrl_cmd * cmd)
{
    int ret = 0;
    struct pisces_early_ringbuf * ctrl_ringbuf;
    ctrl_ringbuf = enclave->ctrl.ctrl_ringbuf;

    ret = pisces_early_ringbuf_write(ctrl_ringbuf, (u8 *) cmd, 
            sizeof(struct pisces_ctrl_cmd));
    if (ret < 0) {
        printk(KERN_WARNING "Pisces controll buffer full!\n");
    }

    return ret;
}
int 
ctrl_recv(struct pisces_enclave * enclave, 
        struct pisces_ctrl_cmd * cmd)
{
    int ret = 0;
    struct pisces_early_ringbuf * ctrl_ringbuf;
    ctrl_ringbuf = enclave->ctrl.ctrl_ringbuf;

    ret = pisces_early_ringbuf_read(ctrl_ringbuf, (u8 *) cmd, 
            sizeof(struct pisces_ctrl_cmd));
    if (ret < 0) {
        printk(KERN_WARNING "Pisces controll read abort: empty buffer!\n");
    }

    return ret;
}

#endif


/*
 * Blocking ctrl buf read
 * Block if buffer is empty
 */
static int
_pisces_ctrl_buf_read(struct pisces_ctrl *ctrl,
        struct pisces_ctrl_cmd * cmd)
{
    unsigned long flags;
    int ret = 0;

    spin_lock_irqsave(&ctrl->lock, flags);

    if (pisces_early_ringbuf_is_empty(ctrl->buf)) {
        printk(KERN_WARNING "Block on reading empty ctrl buffer\n");

        spin_unlock_irqrestore(&ctrl->lock, flags);
        if (wait_event_interruptible(ctrl->waitq,
                    !pisces_early_ringbuf_is_empty(ctrl->buf)))
            return -ERESTARTSYS;
        spin_lock_irqsave(&ctrl->lock, flags);
    }

    ret = pisces_early_ringbuf_read(ctrl->buf, (u8 *) cmd,
            sizeof(struct pisces_ctrl_cmd));

    if (ret == -1) {
        printk(KERN_WARNING "False wakeup in _pisces_ctrl_buf_read");
    }

    spin_unlock_irqrestore(&ctrl->lock, flags);
    return ret;
}


/*
 * Report Error if buffer is full
 */
static int
_pisces_ctrl_buf_write(struct pisces_ctrl *ctrl,
        struct pisces_ctrl_cmd * cmd)
{
    unsigned long flags;
    int ret = 0;

    spin_lock_irqsave(&ctrl->lock, flags);

    if (pisces_early_ringbuf_is_full(ctrl->buf)) {
        printk(KERN_ERR "Error on write to full ctrl buffer\n");

        spin_unlock_irqrestore(&ctrl->lock, flags);
        return -1;
    }

    ret =  pisces_early_ringbuf_write(ctrl->buf, (u8 *) cmd,
            sizeof(struct pisces_ctrl_cmd));

    spin_unlock_irqrestore(&ctrl->lock, flags);
    return ret;
}


int pisces_ctrl_send(struct pisces_enclave * enclave,
        struct pisces_ctrl_cmd * cmd)
{
    int ret  = 0;

    ret = _pisces_ctrl_buf_write(&enclave->ctrl_out, cmd);

    /* Notify enclave to process request */
    if (ret == 0) {
        send_enclave_xcall(enclave->boot_cpu);
    }

    return ret;
}

/*
 * Blocking ctrl receive
 */
int
pisces_ctrl_recv(struct pisces_enclave * enclave,
        struct pisces_ctrl_cmd * cmd)
{
    return _pisces_ctrl_buf_read(&enclave->ctrl_in, cmd);
}


int pisces_ctrl_add_cpu(struct pisces_enclave * enclave, int apicid)
{
    struct pisces_ctrl_cmd cmd;

    memset(&cmd, 0, sizeof(struct pisces_ctrl_cmd));
    cmd.cmd = PISCES_ENCLAVE_ADD_CPU;
    cmd.arg1 = apicid;

    pisces_ctrl_send(enclave, &cmd);

    return 0;
}
