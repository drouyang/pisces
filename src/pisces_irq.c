/* 
 * IRQ request/release protocols. IRQs are associated with IPI vectors for cross-enclave
 * notifications
 *
 * (c) Brian Kocoloski, 2014 (briankoco@cs.pitt.edu)
 *
 */

#include <linux/version.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/pci.h>
#include <asm/ipi.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,1,0)
#include <asm/irq_cfg.h>
#endif

#include "enclave.h"
#include "pisces_irq.h"

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,1,0)
static void 
mask_lapic_irq(unsigned int irq) 
{
    unsigned long v = apic_read(APIC_LVT0);
    apic_write(APIC_LVT0, v | APIC_LVT_MASKED);
}

static void 
unmask_lapic_irq(unsigned int irq) 
{
    unsigned long v = apic_read(APIC_LVT0);
    apic_write(APIC_LVT0, v & ~APIC_LVT_MASKED);
}

static void 
ack_lapic_irq(unsigned int irq) 
{
    ack_APIC_irq();
}

/* lapic_chip (arch/x86/kernel/apic/io_apic.c */
static struct irq_chip 
ipi_chip =
{
    .name   = "Pisces-IPI",
    .mask   = mask_lapic_irq,
    .unmask = unmask_lapic_irq,
    .ack    = ack_lapic_irq,
};
#else
static void
mask_lapic_irq(struct irq_data * data)
{
    unsigned long v = apic_read(APIC_LVT0);
    apic_write(APIC_LVT0, v | APIC_LVT_MASKED);
}

static void
unmask_lapic_irq(struct irq_data * data)
{
    unsigned long v = apic_read(APIC_LVT0);
    apic_write(APIC_LVT0, v & ~APIC_LVT_MASKED);
}

static void
ack_lapic_irq(struct irq_data * data)
{
    ack_APIC_irq();
}

/* lapic_chip (arch/x86/kernel/apic/io_apic.c */
static struct irq_chip 
ipi_chip =
{
    .name       = "Pisces-IPI",
    .irq_mask   = mask_lapic_irq,
    .irq_unmask = unmask_lapic_irq,
    .irq_ack    = ack_lapic_irq,
};
#endif


static int
pisces_alloc_irq(void)
{
    int irq;

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,1,0) 
    irq = create_irq();

    if (irq > 0) {
        struct irq_desc * desc = irq_to_desc(irq);
        if (desc == NULL) {
            printk(KERN_ERR "No desc for irq %d\n", irq);
            destroy_irq(irq);
            return -1;
        }

        desc->status &= ~IRQ_LEVEL;
        set_irq_chip_and_handler(irq, &ipi_chip, handle_edge_irq);
    }
#else
    irq = irq_alloc_hwirq(-1);

    if (irq > 0) {
        irq_clear_status_flags(irq, IRQ_LEVEL);
        irq_set_chip_and_handler(irq, &ipi_chip, handle_edge_irq);
    }
#endif

    return irq;
}

static void
pisces_free_irq(int irq)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,1,0) 
    destroy_irq(irq);
#else
    irq_free_hwirq(irq);
#endif
}

int 
pisces_request_irq(irqreturn_t (*callback)(int, void *),
                  void      * priv_data)
{
    int  irq = 0;
    char name[16];

    /* Allocate hardware irq */
    irq = pisces_alloc_irq();
    if (irq < 0) {
        printk(KERN_ERR "Cannot allocate irq\n");
        return irq;
    }

    /* Request irq callback */
    memset(name, 0, 16);
    snprintf(name, 16, "pisces-%d", irq);

    if (request_irq(irq, callback, 0, name, priv_data) != 0) {
        printk(KERN_ERR "Unable to request callback for irq %d\n", irq);
        pisces_free_irq(irq);
        return -1;
    }

    return irq;
}

int
pisces_release_irq(int    irq,
                   void * priv_data) 
{
    /* Free Linux irq handler */
    free_irq(irq, priv_data);

    /* Free hardware irq */
    pisces_free_irq(irq);

    return 0;
}

int
pisces_irq_to_vector(int irq)
{
    struct irq_cfg * cfg;

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,1,0)
    {
        struct irq_desc * desc = irq_to_desc(irq);
        if (desc == NULL) {
            printk(KERN_ERR "No desc for irq %d\n", irq);
            return -1;
        }

        cfg = (struct irq_cfg *)desc->chip_data;
    }
#else
    cfg = irq_get_chip_data(irq);
#endif

    if (cfg == NULL) {
        printk(KERN_ERR "No chip data for irq %d\n", irq);
        return -1;
    }

    return cfg->vector;
}

static void
pisces_send_ipi_to_apic(unsigned int apic_id, 
                        unsigned int vector)
{
    unsigned long flags = 0;

    local_irq_save(flags);
    {
        __default_send_IPI_dest_field(apic_id, vector, APIC_DEST_PHYSICAL);
    }
    local_irq_restore(flags);
}

static void
pisces_send_ipi(unsigned int cpu_id,
                unsigned int vector)
{
    pisces_send_ipi_to_apic(apic->cpu_present_to_apicid(cpu_id), vector);
}

void
pisces_send_enclave_ipi(struct pisces_enclave * enclave,
                        unsigned int            vector)
{
    pisces_send_ipi(enclave->boot_cpu, vector);
}
