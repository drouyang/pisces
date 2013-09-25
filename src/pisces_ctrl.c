#include "pisces.h"
#include "pisces_ctrl.h"
#include "enclave.h"
#include "enclave_xcall.h"

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

int 
pisces_ctrl_init(
        struct pisces_enclave * enclave, 
        struct pisces_early_ringbuf * ringbuf) {

    enclave->ctrl.ctrl_ringbuf = ringbuf;
    pisces_early_ringbuf_init(ringbuf);

    return 0;
}

void pisces_ctrl_add_cpu(struct pisces_enclave * enclave, int apicid)
{
    struct pisces_ctrl_cmd cmd;

    memset(&cmd, 0, sizeof(struct pisces_ctrl_cmd));
    cmd.cmd = PISCES_ENCLAVE_ADD_CPU;
    cmd.arg1 = apicid;

    if (ctrl_send(enclave, &cmd) ==  0) {
        /* Notify enclave with IPI */
        send_enclave_xcall(apicid);
    }
}
