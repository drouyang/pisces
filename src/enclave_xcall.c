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
#include"enclave_xcall.h"
#include"pisces_ctrl.h"

extern struct pisces_enclave * enclave_map[MAX_ENCLAVES];

void (**linux_x86_platform_ipi_callback)(void) = NULL;

/*
 * Pisces controll command notification handler
 * We do not know the source enclave, so we wakeup all
 * processes waiting for a cmd
 */
static void enclave_xcall_handler(void)
{
    struct pisces_ctrl_cmd;
    struct pisces_enclave * enclave = NULL;
    int i;

    for(i = 0; i < MAX_ENCLAVES; i++) {
        enclave = enclave_map[i];
        if (enclave != NULL) {
            wake_up_interruptible(&(enclave->ctrl_in.waitq));
        }
    }
    printk(KERN_INFO "Enclave xcall received.\n");
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

/*
 * Hack: use phycial dest to send IPI to Kitten
 * Linux by default uses logical mode
 */
void send_enclave_xcall(int apicid)
{
    //apic->send_IPI_mask(cpumask_of(1), PISCES_ENCLAVE_XCALL_VECTOR);
    __default_send_IPI_dest_field(apicid, PISCES_ENCLAVE_XCALL_VECTOR, APIC_DEST_PHYSICAL);
}


