#include<linux/kernel.h>
#include<linux/module.h>

#include"pisces.h"
#include"pisces_loader.h"
#include"pisces_dev.h"      /* device file ioctls*/
#include"domain_xcall.h"

#include<linux/irq.h>
#include<linux/interrupt.h>
#include<asm/irq.h>


#define AUTHOR "Jiannan Ouyang <ouyang@cs.pitt.edu>"
#define DESC "Pisces: native os consolidation"


//extern bootstrap_pgt_t *bootstrap_pgt;

unsigned long mem_base = 0x8000000;
module_param(mem_base, ulong, S_IRWXU);

unsigned long mem_len = 0x8000000;
module_param(mem_len, ulong, S_IRWXU);

unsigned long cpu_id = 1;
module_param(cpu_id, ulong, S_IRWXU);

char *kernel_path = "/home/ouyang/pisces/kitten-1.3.0/vmlwk.bin";
module_param(kernel_path, charp, S_IRWXU);

char *initrd_path = "/home/ouyang/pisces/kitten-1.3.0/init_task";
module_param(initrd_path, charp, S_IRWXU);

char *boot_cmd_line = "console=pisces init_argv=\"one two three\" init_envp=\"a=1 b=2 c=3\"";
module_param(boot_cmd_line, charp, S_IRWXU);

unsigned long mpf_found_addr = 0xffffffff81bd1c00;
module_param(mpf_found_addr, ulong, S_IRWXU);


// This pointer is usable only after the memory layout has been setup
struct shared_info_t *shared_info;

static int pisces_init(void)
{
    int ret = 0; 

    printk(KERN_INFO "PISCES: module loaded\n");
        /*
    {
        int i;
        struct irq_desc *desc;

        i = 235;
        desc = irq_to_desc(i);
        printk(KERN_INFO "%d: status=%08x\n",
                i, (u32) desc->status_use_accessors);
        for_each_irq_desc(i, desc) {
            printk(KERN_INFO "%d: status=%08x\n",
                    i, (u32) desc->status_use_accessors);
        }
    
    }
        */


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

    start_instance();

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
