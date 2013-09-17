#include "pisces_ctrl.h"
#include "enclave.h"

int 
ctrl_send(struct pisces_early_ringbuf * ctrl_ringbuf, 
        struct pisces_ctrl_cmd * cmd)
{
    pisces_early_ringbuf_write(ctrl_ringbuf, (u8 *) cmd, 
            sizeof(struct pisces_ctrl_cmd));

    return 0;
}
int 
ctrl_recv(struct pisces_early_ringbuf * ctrl_ringbuf, 
        struct pisces_ctrl_cmd * cmd)
{
    pisces_early_ringbuf_read(ctrl_ringbuf, (u8 *) cmd, 
            sizeof(struct pisces_ctrl_cmd));

    return 0;
}

int 
pisces_ctrl_init(
        struct pisces_enclave * enclave, 
        struct pisces_early_ringbuf * ctrl_ringbuf) {

    enclave->ctrl.ctrl_ringbuf = ctrl_ringbuf;
    pisces_early_ringbuf_init(ctrl_ringbuf);

    return 0;
}
