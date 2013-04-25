/*
 * Pisces cross domain call
 * Author: Jianan Ouyang (ouyang@cs.pitt.edu)
 * Date: 04/2013
 */
#include<linux/interrupt.h>
#include<linux/irq.h>
#include<linux/pci.h>
#include<linux/export.h>
#include<asm/desc.h>
#include"pisces.h"
#include"domain_xcall.h"

int irq = 0;
int vector = 247;

/*
 * Domain cross call is handled like a syscall.
 * Caller apic_id, xcall_id and params are passed through shared memory
 */
/*
static irqreturn_t domain_xcall_interrupt(int irq, void *dev_id)
{
    printk(KERN_INFO "PISCES: domain xcall received\n");

    return IRQ_HANDLED;
}
*/

static void domain_xcall_IPI_callback(void)
{
    printk(KERN_INFO "PISCES: domain xcall received\n");
}

extern struct cdev c_dev;  
/* 
 * Request an irq vector from linux as a shared irq vector across domains
 * This vector number should be available in OSes from all domains
 * A handler is registered for this vector
 */
int domain_xcall_init(void)
{
#if 0
    static struct irq_chip pisces_irq_chip;
    int ret;
    int (*create_irq)(void); 
    
    create_irq= (int (*)(void))0xffffffff81029240;
    

    pisces_irq_chip = dummy_irq_chip;
    pisces_irq_chip.name = " PISCES-XCALL";

    irq = create_irq();

    /*
    ret = irq_alloc_descs(irq, 0, 1, numa_node_id());

    if (ret < 0) {
        printk(KERN_INFO "PISCES: fail to allocate irq %d, error code %d\n", irq, ret);
        return -1;
    } 
    */

    irq_set_chip_and_handler(irq, &pisces_irq_chip, handle_simple_irq);

    ret = request_irq(irq, 
            domain_xcall_interrupt,
            0,
            "pisces",
            NULL);

    enable_irq(irq);

    if (ret) {
        printk(KERN_INFO "PISCES: failed to request_irq %d, error code %d\n", 
                irq,
                ret);
        return -1;
    } 

    vector = irq + 16;

    printk(KERN_INFO "PISCES: allocate irq %d for domain_xcall\n", irq);
    //alloc_intr_gate(PISCES_DOMAIN_XCALL_VECTOR, domain_xcall_interrupt);
#endif
    void (**x86_platform_ipi_callback)(void);

    x86_platform_ipi_callback = (void (**)(void))0xffffffff81bbb148;
    *x86_platform_ipi_callback = domain_xcall_IPI_callback;

    return 0;
}

void domain_xcall_exit(void)
{
    //free_irq(irq, NULL);
    //irq_free_descs(irq, 1);
    //printk(KERN_INFO "PISCES: free irq %d for domain_xcall\n", irq);
}

