/* Pisces: multi-instance OS framework
 * (c) Jiannan Ouyang, 2013
 * (c) Jack Lange, 2013
 */
#include<linux/kernel.h>
#include<linux/module.h>


#include"pisces_dev.h"      /* device file ioctls*/
#include"domain_xcall.h"

#include<linux/irq.h>
#include<linux/interrupt.h>
#include<asm/irq.h>


#define AUTHOR "Jiannan Ouyang <ouyang@cs.pitt.edu>"
#define DESC "Pisces: multi-instance OS framework"


unsigned long cpu_id = 1;
module_param(cpu_id, ulong, S_IRWXU);
  

unsigned long mpf_found_addr = 0xffffffff81bd1c00;
module_param(mpf_found_addr, ulong, S_IRWXU);



static int pisces_init(void) {
    int ret = 0; 

    printk(KERN_INFO "PISCES: module loaded\n");

    ret = device_init();
    if (ret < 0) {
        printk(KERN_INFO "PISCES: device file registeration failed\n");
        return 0;
    }

    ret = domain_xcall_init();
    if (ret < 0) {
        printk(KERN_INFO "PISCES: domain_xcall_init failed\n");
        return 0;
    }


    return 0;
}

static void pisces_exit(void)
{
    device_exit();
    printk(KERN_INFO "PISCES: module unloaded\n");
    return;
}

module_init(pisces_init);
module_exit(pisces_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR(AUTHOR);
MODULE_DESCRIPTION(DESC);
