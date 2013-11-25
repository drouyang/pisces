#include "pisces_boot_params.h"
#include "enclave.h"
#include "pisces_data.h"
#include "ipi.h"


#include <linux/slab.h>



static int pisces_send_data(struct pisces_cmd_buf * cmd_buf, void * data,
            u32 buf_size, int is_cmd) {

    struct pisces_cmd * cmd = NULL;
    struct pisces_resp * resp = NULL;
    u32 written = 0;
    u32 total = 0;
    u32 staging_len = 0;

    if (is_cmd) {
        cmd = (struct pisces_cmd *)data;
        total = cmd_buf->cmd.data_len;
    } else {
        resp = (struct pisces_resp *)data;
        total = cmd_buf->resp.data_len;
    }

    for (written = 0; written < total; written += staging_len) {
        while (cmd_buf->staging == 1) {
            __asm__ __volatile__ ("":::"memory");
        }

        staging_len = buf_size;
        if (staging_len > total - written) {
            staging_len = total - written;
        }

        if (is_cmd) {
            memcpy(cmd_buf->cmd.data,
                cmd->data + (uintptr_t)written, 
                staging_len); 
        } else {
            memcpy(cmd_buf->resp.data,
                resp->data + (uintptr_t)written, 
                staging_len); 
        }

        cmd_buf->staging_len = staging_len;
        cmd_buf->staging = 1;
        __asm__ __volatile__ ("":::"memory");
    }

    return 0;
}

static int pisces_recv_data(struct pisces_cmd_buf * cmd_buf, void * data, 
            int is_cmd) {

    struct pisces_cmd * cmd = NULL;
    struct pisces_resp * resp = NULL;

    u32 read = 0;
    u32 total = 0;
    u32 staging_len = 0;

    if (is_cmd) {
        cmd = (struct pisces_cmd *)data;
        total = cmd_buf->cmd.data_len;
    } else {
        resp = (struct pisces_resp *)data;
        total = cmd_buf->resp.data_len;
    }

    for (read = 0; read < total; read += staging_len) {
        /* Always wait for next stage */
        cmd_buf->staging = 0;
        while (cmd_buf->staging == 0) {
            __asm__ __volatile__ ("":::"memory");
        }

        staging_len = cmd_buf->staging_len;

        if (is_cmd) {
            memcpy(cmd->data + (uintptr_t)read,
                    cmd_buf->cmd.data,
                    staging_len);
        } else {
            memcpy(resp->data + (uintptr_t)read,
                    cmd_buf->resp.data,
                    staging_len);
        }
    }

    return 0;
}

int pisces_send_cmd(struct pisces_enclave * enclave, struct pisces_cmd * cmd) {
    struct pisces_ctrl * ctrl = &(enclave->ctrl);
    struct pisces_cmd_buf * cmd_buf = ctrl->cmd_buf;
    struct pisces_boot_params * boot_params = __va(enclave->bootmem_addr_pa);
    u32 buf_size = 0;
    int ret = 0;

    if (cmd_buf->ready == 0) {
        printk(KERN_ERR "Attempted control transfer to unready enclave OS\n");
        return -1;
    }

    if (cmd_buf->staging || cmd_buf->active) {
        printk(KERN_ERR "CMD is active. This should be impossible\n");
        return -1;
    }

    /* Signal that command is staging */
    cmd_buf->active = 0;
    cmd_buf->completed = 0;

    buf_size = boot_params->control_buf_size -
            sizeof(struct pisces_cmd_buf);

    /* Copy header */
    memcpy(&(cmd_buf->cmd), cmd, sizeof(struct pisces_cmd));

    /* Send the IPI here */
    cmd_buf->staging = 1;
    mb();
    __asm__ __volatile__ ("":::"memory");

    printk("Sending IPI to enclave cpu %d\n", cmd_buf->enclave_cpu);
    pisces_send_ipi(enclave, cmd_buf->enclave_cpu, cmd_buf->enclave_vector);
    printk("IPI completed\n");

    /* Send the data */
    ret = pisces_send_data(cmd_buf, (void *)cmd, buf_size, 1);

    /* Wait for active flag to be 1 */
    while (cmd_buf->active == 0) {
        __asm__ __volatile__ ("":::"memory");
    }

    return ret;
}

int pisces_recv_resp(struct pisces_enclave * enclave, struct pisces_resp ** resp_p,
        int atomic) 
{
    struct pisces_ctrl * ctrl = &(enclave->ctrl);
    struct pisces_cmd_buf * cmd_buf = ctrl->cmd_buf;
    int ret;
    gfp_t flags = 0;

    if (atomic)
        flags = GFP_ATOMIC;
    else 
        flags = GFP_KERNEL;

    *resp_p = kmalloc(sizeof(struct pisces_resp) + cmd_buf->resp.data_len, flags);
    if (!*resp_p) {
        printk(KERN_ERR "Cannot allocate buffer for response!\n");
        return -ENOMEM;
    }   

    /* Copy header */
    memcpy(*resp_p, &(cmd_buf->resp), sizeof(struct pisces_resp));

    /* Receive the data */
    ret = pisces_recv_data(cmd_buf, (void *)*resp_p, 0);

    /* Signal that control channel is no longer active */
    cmd_buf->active = 0;
    cmd_buf->completed = 0;
    cmd_buf->staging = 0;
    return ret;
}

int pisces_recv_cmd(struct pisces_enclave * enclave, struct pisces_cmd ** cmd_p,
            int atomic) 
{
    struct pisces_lcall * lcall = &(enclave->lcall);
    struct pisces_cmd_buf * cmd_buf = lcall->cmd_buf;
    int ret = 0;
    gfp_t flags = 0;

    if (atomic)
        flags = GFP_ATOMIC;
    else 
        flags = GFP_KERNEL;

    *cmd_p = kmalloc(sizeof(struct pisces_cmd) + cmd_buf->cmd.data_len, flags);
    if (!*cmd_p) {
        printk(KERN_ERR "Cannot allocate buffer for command!\n");
        return -ENOMEM;
    }   

    /* Copy header */
    memcpy(*cmd_p, &(cmd_buf->cmd), sizeof(struct pisces_cmd));

    /* Receive the data */
    ret = pisces_recv_data(cmd_buf, (void *)*cmd_p, 1);

    cmd_buf->active = 1;
    cmd_buf->staging = 0;
    return ret;
}

int pisces_send_resp(struct pisces_enclave * enclave, struct pisces_resp * resp) {
    struct pisces_lcall * lcall = &(enclave->lcall);
    struct pisces_cmd_buf * cmd_buf = lcall->cmd_buf;
    struct pisces_boot_params  * boot_params = __va(enclave->bootmem_addr_pa);
    int ret = 0;

    u32 buf_size = boot_params->longcall_buf_size -
            sizeof(struct pisces_cmd_buf);

    /* Copy header */
    memcpy(&(cmd_buf->resp), resp, sizeof(struct pisces_resp));

    /* Enclave waits for completed flag */
    cmd_buf->staging = 1;
    cmd_buf->completed = 1;

    /* Send the data */
    ret = pisces_send_data(cmd_buf, (void *)resp, buf_size, 0);

    /* Wait for active flag to be 0 */
    while (cmd_buf->active) {
        __asm__ __volatile__ ("":::"memory");
    }

    return ret;
}
