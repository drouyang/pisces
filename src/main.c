#include<linux/kernel.h>
#include<linux/module.h>

#include"pisces.h"
#include"pisces_loader.h"
#include"pisces_dev.h"      /* device file ioctls*/



/*
#include<linux/init.h>
#include<linux/smp.h>
#include<linux/sched.h>
#include<linux/percpu.h>
#include<linux/bootmem.h>
#include<linux/tboot.h>
#include<linux/gfp.h>
#include<linux/cpuidle.h>

#include<asm/apic.h>
#include<asm/desc.h>
#include<asm/idle.h>
#include<asm/cpu.h>
#include<asm/uaccess.h>
*/


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

struct shared_info_t *shared_info;


static int pisces_init(void)
{
    int ret = 0; 

    printk(KERN_INFO "PISCES: module loaded\n");

    ret = device_init();
    if (ret < 0) {
        printk(KERN_INFO "PISCES: device file registeration failed\n");
        return -1;
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
