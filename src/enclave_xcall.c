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
#include"enclave_xcall.h"

void (**linux_x86_platform_ipi_callback)(void) = NULL;

static void enclave_xcall_handler(void)
{
    printk(KERN_INFO "PISCES: enclave xcall received\n");
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


