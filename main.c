#include<linux/module.h>
#include<linux/kernel.h>
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
#include<asm/realmode.h>
#include<asm/idle.h>
#include<asm/cpu.h>

#include"inc/gemini.h"

#define AUTHOR "Jiannan Ouyang <ouyang@cs.pitt.edu>"
#define DESC "Gemini: native os consolidation"

//extern unsigned long gemini_trampoline;
unsigned long get_trampoline_start(void);

void __init start_secondary(void)
{
    printk(KERN_INFO "GEMINI: jumped back\n");
    native_halt();
}
static int foo(int cpu) 
{
    int ret = 0;
    int apicid = apic->cpu_present_to_apicid(1);
    unsigned long start_ip = real_mode_header->trampoline_start;
    /*
    struct task_struct *idle;

    idle = (struct task_struct *)kmalloc(sizeof(struct task_struct), GFP_KERNEL);
    init_idle(idle, cpu);
    if (IS_ERR(idle)) {
        printk(KERN_INFO "GEMINI: idle_thread_get error %p\n", idle);
    }

    idle->thread.sp = (unsigned long) (((struct pt_regs *) (THREAD_SIZE + task_stack_page(idle))) - 1);
    per_cpu(current_task, cpu) = idle;
    clear_tsk_thread_flag(idle, TIF_FORK);
    initial_gs = per_cpu_offset(cpu);
    per_cpu(kernel_stack, cpu) = 
        (unsigned long) task_stack_page(idle) -
        KERNEL_STACK_OFFSET + THREAD_SIZE;
    stack_start = idle->thread.sp;
        */
    //early_gdt_descr.address = (unsigned long)per_cpu(gdt_page, cpu).gdt;
    printk(KERN_INFO "GEMINI: start_ip %lx; target ip: %lx\n", start_ip, (unsigned long)start_secondary);

    initial_code = (unsigned long) start_secondary;

    //ret = wakeup_secondary_cpu_via_init(apicid, gemini_trampoline);
    ret = wakeup_secondary_cpu_via_init(apicid, start_ip);
    return ret;

}
static int __init gemini_init(void)
{
        int ret = 0;

	printk(KERN_INFO "GEMINI: loaded\n");

        ret = foo(1);   

        printk(KERN_INFO "GEMINI: smp call return: %d\n", ret);
        
	return 0;
}

static void __exit gemini_exit(void)
{
	printk(KERN_INFO "GEMINI: unloaded\n");
	return;
}

int entry(void)
{
    while(1);
}

module_init(gemini_init);
module_exit(gemini_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR(AUTHOR);
MODULE_DESCRIPTION(DESC);
