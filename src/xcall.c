/*
 * Pisces cross domain call
 * Author: Jianan Ouyang (ouyang@cs.pitt.edu)
 * Date: 04/2013
 */
#include<linux/interrupt.h>
#include<linux/irq.h>
#include<linux/pci.h>
#include<linux/sched.h>
#include<linux/wait.h>
#include<linux/export.h>
#include<asm/desc.h>
#include<asm/ipi.h>
#include"pisces.h"
#include"enclave.h"
#include"xcall.h"
#include"pisces_ctrl.h"


void (**linux_x86_platform_ipi_callback)(void) = NULL;

static void enclave_xcall_handler(void)
{

}

int enclave_xcall_init(void)
{
    if (linux_x86_platform_ipi_callback == NULL) {
        return -1;
    } else {
        *linux_x86_platform_ipi_callback = enclave_xcall_handler;
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


