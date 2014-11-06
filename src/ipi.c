/*
 * Routines for registering IPI callbacks.
 *
 * Somehow, Linux does not provide an interface by which IDT vectors can be dynamically
 * allocated. So, our approach is to figure out which vectors are free (by using the Linux
 * 'used_vectors' bitmap) and insert callbacks into the IDT at runtime. All interrupt
 * handlers are loaded dynamically from a custom IDT table 'generic_ipi_table' defined in
 * idt.S. These handlers all direct to the 'do_generic_ipi_handler' function below.
 *
 * It should be noted that registered callbacks now execute in interrupt context, so be
 * careful when registering handlers.
 *
 * (c) Brian Kocoloski, 2014 (briankoco@cs.pitt.edu)
 * (c) Jiannan Ouyang,  2013 (ouyang@cs.pitt.edu)
 *
 */
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/pci.h>
#include <asm/desc.h>
#include <asm/ipi.h>


#include "pisces.h"
#include "enclave.h"
#include "ipi.h"
#include "enclave_ctrl.h"
#include "linux_syms.h"


/* Table of IDT handlers found in idt.S */
extern void (*generic_ipi_table[NR_VECTORS])(void);


/* While anything at or above FIRST_EXTERNAL_VECTOR will work, Linux may have trouble
 * offlining CPUs if we choose lower vectors, as it needs to find a range of free vectors
 * in order to migrate IRQs during CPU offlines
 */
#define PISCES_FIRST_EXTERNAL_VECTOR FIRST_EXTERNAL_VECTOR + 64

#if PISCES_FIRST_EXTERNAL_VECTOR < FIRST_EXTERNAL_VECTOR
#error PISCES_FIRST_EXTENAL_VECTOR MUST BE AT LEAST AT LARGE AS FIRST_EXTERNAL_VECTOR
#endif


struct ipi_callback {
    void  (*callback)(unsigned int vector, void * private_data);
    void * private_data;
    int    allocated;
};

static struct ipi_callback ipi_callbacks[NR_VECTORS];
static DEFINE_SPINLOCK(ipi_lock);
//static int orig_first_system_vector = 0;

static void
__do_generic_ipi_handler(unsigned int vector)
{
    struct ipi_callback * cb = &(ipi_callbacks[vector]);

    if (unlikely(cb->allocated == 0)) {
	pr_emerg_ratelimited("Received IPI on unallocated vector! (%u)\n", vector);
	return;
    }

    cb->callback(vector, cb->private_data);
}

__visible void
do_generic_ipi_handler(struct pt_regs * regs,
                       unsigned int     vector)
{
    struct pt_regs * old_regs = set_irq_regs(regs);
    
    linux_irq_enter();
    linux_exit_idle();

    __do_generic_ipi_handler(vector);

    linux_irq_exit();
    if (vector >= FIRST_EXTERNAL_VECTOR)
	ack_APIC_irq();

    set_irq_regs(old_regs);
}

static void
pisces_set_intr_gate(unsigned int vector)
{
    gate_desc   s;
    void      * idt_handler = (void *)((uintptr_t)(&generic_ipi_table) + (vector * 16));

    pack_gate(&s, GATE_INTERRUPT, (unsigned long)idt_handler, 0, 0, __KERNEL_CS);

    write_idt_entry(linux_idt_table, vector, &s);
}

static int
pisces_register_ipi_vector(void)
{
    unsigned long flags  = 0;
    int           vector = 0;

    spin_lock_irqsave(&ipi_lock, flags);
    {
	/* Find a verctor that Linux/Pisces is not yet using */
        vector = find_next_zero_bit(used_vectors, NR_VECTORS, PISCES_FIRST_EXTERNAL_VECTOR);

	if (vector < NR_VECTORS) {
	    /* Tell Linux the vector is no longer free */
	    set_bit(vector, used_vectors);
	    if (*linux_first_system_vector > vector) { 
		*linux_first_system_vector = vector;
	    }

	    /* Install the ipi handler in the idt */
	    pisces_set_intr_gate(vector);
	} else {
	    vector = -1;
	}

    }
    spin_unlock_irqrestore(&ipi_lock, flags);

    /* Return the vector */
    return vector;
}

static void
pisces_free_ipi_vector(unsigned int vector)
{
    unsigned long flags = 0;

    spin_lock_irqsave(&ipi_lock, flags);
    {
	clear_bit(vector, used_vectors);
	/* TODO: put do_IRQ back in the idt? */
    }
    spin_unlock_irqrestore(&ipi_lock, flags);
}


int 
pisces_request_ipi_vector(void   (*callback)(unsigned int, void *),
                          void * private_data)
{
    struct ipi_callback * cb     = NULL;
    int                   vector = 0;

    vector = pisces_register_ipi_vector();
    if (vector == -1) {
	return -1;
    }

    cb = &(ipi_callbacks[vector]);

    cb->callback     = callback;
    cb->private_data = private_data;
    cb->allocated    = 1;

    return vector;
}

EXPORT_SYMBOL(pisces_request_ipi_vector);


int
pisces_release_ipi_vector(int vector) 
{

    pisces_free_ipi_vector(vector);
    memset(&(ipi_callbacks[vector]), 0, sizeof(struct ipi_callback));

    return 0;
}

EXPORT_SYMBOL(pisces_release_ipi_vector);


int 
pisces_ipi_init(void)
{
    memset(&ipi_callbacks, 0, sizeof(struct ipi_callback) * NR_VECTORS);
    spin_lock_init(&ipi_lock);

    //orig_first_system_vector = *linux_first_system_vector;

    return 0;
}

int
pisces_ipi_deinit(void)
{
//    *linux_first_system_vector = orig_first_system_vector;

    return 0;
}


void
pisces_send_ipi(unsigned int cpu,
                unsigned int vector)
{
    unsigned long flags = 0;

    local_irq_save(flags);
    {
	__default_send_IPI_dest_field(apic->cpu_present_to_apicid(cpu), vector, APIC_DEST_PHYSICAL);
    }
    local_irq_restore(flags);
}

EXPORT_SYMBOL(pisces_send_ipi);

void 
pisces_send_enclave_ipi(struct pisces_enclave * enclave,
		        unsigned int            vector)
{
    return pisces_send_ipi(enclave->boot_cpu, vector);
}



