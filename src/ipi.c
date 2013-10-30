/*
 * Pisces cross domain call
 * Author: Jianan Ouyang (ouyang@cs.pitt.edu)
 * Date: 04/2013
 */
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/pci.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/export.h>
#include <asm/desc.h>
#include <asm/ipi.h>
#include "pisces.h"
#include "enclave.h"
#include "ipi.h"
#include "pisces_ctrl.h"

#include "linux_syms.h"

struct ipi_callback {
    void (*callback)(void * private_data);
    void * private_data;
    struct list_head node;
};

static LIST_HEAD(ipi_callbacks);


int pisces_register_ipi_callback(void (*callback)(void *), void * private_data) {
    struct ipi_callback * new_cb = NULL;

    new_cb = kmalloc(sizeof(struct ipi_callback), GFP_KERNEL);

    if (IS_ERR(new_cb)) {
	printk(KERN_ERR "Error allocating new IPI callback structure\n");
	return -1;
    }

    new_cb->callback = callback;
    new_cb->private_data = private_data;

    list_add_tail(&(new_cb->node), &ipi_callbacks);

    return 0;
}


static void 
platform_ipi_handler(void) {
    struct ipi_callback * iter = NULL;

    printk("Handling IPI callbacks\n");

    list_for_each_entry(iter, &(ipi_callbacks), node) {
	iter->callback(iter->private_data);
    }

    return;
}


int pisces_ipi_init(void)
{
    if (linux_x86_platform_ipi_callback == NULL) {
        return -1;
    } else {
        *linux_x86_platform_ipi_callback = platform_ipi_handler;
        return 0;
    }
}



int pisces_send_ipi(struct pisces_enclave * enclave, int cpu_id, unsigned int vector)
{

    if (cpu_id != 0) {
	printk(KERN_ERR "Currently we only allow sending IPI's to the boot CPU\n");
	return -1;
    }

    __default_send_IPI_dest_field(enclave->boot_cpu, vector, APIC_DEST_PHYSICAL);

    return 0;
}


