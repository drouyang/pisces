/*
 * Pisces cross domain call
 * Author: Jianan Ouyang (ouyang@cs.pitt.edu)
 * Date: 04/2013
 */
#include<linux/interrupt.h>
#include<linux/irq.h>
#include<linux/export.h>
#include"domain_xcall.h"
#include"pisces.h"

static int irq = PISCES_DOMAIN_XCALL_VECTOR;

/*
 * Domain cross call is handled like a syscall.
 * Caller apic_id, xcall_id and params are passed through shared memory
 */
static irqreturn_t domain_xcall_interrupt(int irq, void *dev_id)
{
    printk(KERN_INFO "PISCES: domain xcall received\n");

    return IRQ_HANDLED;
}

extern struct cdev c_dev;  
/* 
 * Request an irq vector from linux as a shared irq vector across domains
 * This vector number should be available in OSes from all domains
 * A handler is registered for this vector
 */
int domain_xcall_init(void)
{
    static struct irq_chip pisces_irq_chip;
    int ret;
    

    irq = irq_alloc_descs(irq, 0, 1, numa_node_id());

    if (irq < 0) {
        printk(KERN_INFO "PISCES: fail to allocate irq %d\n", irq);
        return -1;
    } else {
        printk(KERN_INFO "PISCES: allocate irq %d for domain_xcall\n", irq);
    
    }

    pisces_irq_chip = dummy_irq_chip;
    pisces_irq_chip.name = "pisces";
    irq_set_chip_and_handler(irq, &pisces_irq_chip, handle_simple_irq);

    ret = request_irq(irq, 
            domain_xcall_interrupt,
            IRQF_DISABLED | IRQF_TRIGGER_FALLING,
            "pisces_domain_xcall",
            NULL);

    if (ret) {
        printk(KERN_INFO "PISCES: request_irq %d failed, error code %d\n", 
                PISCES_DOMAIN_XCALL_VECTOR,
                ret);
        return -1;
    } 

    return 0;
}

void domain_xcall_exit(void)
{
    free_irq(irq, NULL);
    irq_free_descs(irq, 1);

}

