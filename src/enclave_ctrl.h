#ifndef _PISCES_CTRL_H_
#define _PISCES_CTRL_H_

#include <linux/wait.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <asm/irq_vectors.h>

#include "ctrl_ioctl.h"




struct pisces_enclave;
struct pisces_xbuf_desc;

struct pisces_ctrl {
    struct mutex lock;

    struct pisces_xbuf_desc * xbuf_desc;
} __attribute__((packed));


int pisces_ctrl_init(struct pisces_enclave * enclave);
int pisces_ctrl_deinit(struct pisces_enclave * enclave);
int pisces_ctrl_connect(struct pisces_enclave * enclave);

int ctrl_add_mem(struct pisces_enclave * enclave, struct memory_range * reg);
int ctrl_add_cpu(struct pisces_enclave * enclave, u64 cpu_id);
int ctrl_add_pci(struct pisces_enclave * enclave, struct pisces_pci_spec * pci_spec);


#endif
