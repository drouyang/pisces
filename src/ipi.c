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
static DEFINE_SPINLOCK(ipi_lock);


int pisces_register_ipi_callback(void (*callback)(void *), void * private_data) {
    struct ipi_callback * new_cb = NULL;
    unsigned long flags;

    new_cb = kmalloc(sizeof(struct ipi_callback), GFP_KERNEL);

    if (IS_ERR(new_cb)) {
	printk(KERN_ERR "Error allocating new IPI callback structure\n");
	return -1;
    }

    new_cb->callback = callback;
    new_cb->private_data = private_data;

    spin_lock_irqsave(&ipi_lock, flags);
    list_add_tail(&(new_cb->node), &ipi_callbacks);
    spin_unlock_irqrestore(&ipi_lock, flags);

    return 0;
}


int pisces_remove_ipi_callback(void (*callback)(void *), void * private_data) {
    struct ipi_callback * cb = NULL;
    struct ipi_callback * tmp = NULL;
    unsigned long flags;

    spin_lock_irqsave(&ipi_lock, flags);
    list_for_each_entry_safe(cb, tmp, &ipi_callbacks, node) {
	if ((cb->callback == callback) && 
	    (cb->private_data == private_data)) {
	    list_del(&(cb->node));
	    kfree(cb);
	}
    }
    spin_unlock_irqrestore(&ipi_lock, flags);

    return 0;
}


static void 
platform_ipi_handler(void) {
    struct ipi_callback * iter = NULL;
    unsigned long flags;
//    printk("Handling IPI callbacks\n");

    spin_lock_irqsave(&ipi_lock, flags);
    list_for_each_entry(iter, &(ipi_callbacks), node) {
	iter->callback(iter->private_data);
    }
    spin_unlock_irqrestore(&ipi_lock, flags);

    return;
}


int pisces_ipi_init(void)
{

   INIT_LIST_HEAD(&ipi_callbacks);

    if (linux_x86_platform_ipi_callback == NULL) {
        return -1;
    } else {
        *linux_x86_platform_ipi_callback = platform_ipi_handler;
        return 0;
    }
}



int pisces_send_ipi(struct pisces_enclave * enclave, int cpu_id, unsigned int vector)
{
    unsigned long flags;

    if (cpu_id != 0) {
	printk(KERN_ERR "Currently we only allow sending IPI's to the boot CPU\n");
	return -1;
    }

    printk("Sending IPI %u to CPU %d (APIC=%d)\n", vector, enclave->boot_cpu, apic->cpu_present_to_apicid(enclave->boot_cpu));

    local_irq_save(flags);
    __default_send_IPI_dest_field(apic->cpu_present_to_apicid(enclave->boot_cpu), vector, APIC_DEST_PHYSICAL);
    local_irq_restore(flags);

    return 0;
}


