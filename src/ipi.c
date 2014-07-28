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
#include <linux/kthread.h>
#include <asm/desc.h>
#include <asm/ipi.h>


#include "pisces.h"
#include "enclave.h"
#include "ipi.h"
#include "enclave_ctrl.h"

#include "linux_syms.h"

struct ipi_callback {
    void (*callback)(void * private_data);
    void * private_data;
    struct list_head node;
};


static LIST_HEAD(ipi_callbacks);
static DEFINE_SPINLOCK(ipi_lock);

static wait_queue_head_t    notifier_waitq;
static struct task_struct * notifier_thread = NULL;

static u64 ipis_recvd    = 0;
static u64 notifications = 0;


int 
pisces_register_ipi_callback(void (*callback)(void *),
			     void * private_data) 
{
    struct ipi_callback * new_cb = NULL;
    unsigned long flags;

    new_cb = kmalloc(sizeof(struct ipi_callback), GFP_KERNEL);

    if (IS_ERR(new_cb)) {
	printk(KERN_ERR "Error allocating new IPI callback structure\n");
	return -1;
    }

    new_cb->callback     = callback;
    new_cb->private_data = private_data;

    spin_lock_irqsave(&ipi_lock, flags);
    {
	list_add_tail(&(new_cb->node), &ipi_callbacks);
    }
    spin_unlock_irqrestore(&ipi_lock, flags);

    return 0;
}


int 
pisces_remove_ipi_callback(void (*callback)(void *), 
			   void * private_data) 
{
    struct ipi_callback * cb = NULL;
    struct ipi_callback * tmp = NULL;
    unsigned long flags;

    spin_lock_irqsave(&ipi_lock, flags);
    {

	list_for_each_entry_safe(cb, tmp, &ipi_callbacks, node) {
	    if ((cb->callback == callback) && 
		(cb->private_data == private_data)) {
		list_del(&(cb->node));
		kfree(cb);
	    }
	}
    }
    spin_unlock_irqrestore(&ipi_lock, flags);

    return 0;
}


int 
notifier_thread_fn(void * arg) 
{
    struct ipi_callback * iter = NULL;
//    printk("Handling IPI callbacks\n");

    while (1) {
	wait_event_interruptible(notifier_waitq, (notifications < ipis_recvd));
	    
	if (notifications < ipis_recvd) {

	    notifications = ipis_recvd;

	    spin_lock(&ipi_lock);
	    {
		list_for_each_entry(iter, &(ipi_callbacks), node) {
		    iter->callback(iter->private_data);
		}
	    }
	    spin_unlock(&ipi_lock);
	}
    }
   
    return 0;
}


static void 
platform_ipi_handler(void) 
{
    ipis_recvd++;

    mb();
    wake_up_interruptible(&notifier_waitq);

    return;
}


int 
pisces_ipi_init(void)
{

    if (linux_x86_platform_ipi_callback == NULL) {
	panic("Platform IPI symbol not initialized\n");
        return -1;
    }

   INIT_LIST_HEAD(&ipi_callbacks);
   spin_lock_init(&ipi_lock);
   init_waitqueue_head(&notifier_waitq);

   notifier_thread = kthread_create(notifier_thread_fn, NULL, "pisces_notifyd");
   wake_up_process(notifier_thread);

   *linux_x86_platform_ipi_callback = platform_ipi_handler;
   
   return 0;
}



int 
pisces_send_ipi(struct pisces_enclave * enclave,
		int                     cpu_id, 
		unsigned int            vector)
{
    unsigned long flags;

    if (cpu_id != 0) {
	printk(KERN_ERR "Currently we only allow sending IPI's to the boot CPU\n");
	return -1;
    }

    local_irq_save(flags);
    {
	__default_send_IPI_dest_field(apic->cpu_present_to_apicid(enclave->boot_cpu), vector, APIC_DEST_PHYSICAL);
    }
    local_irq_restore(flags);

    return 0;
}


