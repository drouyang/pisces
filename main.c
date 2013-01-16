#include<linux/module.h>
#include<linux/kernel.h>
#include<linux/init.h>

#include<asm/apic.h>

#define AUTHOR "Jiannan Ouyang <ouyang@cs.pitt.edu>"
#define DESC "Gemini: native os consolidation"

void gemini_print(void)
{
    printk(KERN_INFO "GEMINI: IPI success!\n");
}
static int __init gemini_init(void)
{
        int ret = 0;
        //int apicid = apic->cpu_present_to_apicid(1);

	printk(KERN_INFO "GEMINI: loaded\n");

        preempt_disable();
        ret = smp_call_function_single(1, gemini_print, NULL, 0);
        //apic->send_IPI_mask(cpumask_of(1), CALL_FUNCTION_SINGLE_VECTOR);
        preempt_enable();

        printk(KERN_INFO "GEMINI: smp call return: %d\n", ret);
        
	return 0;
}

static void __exit gemini_exit(void)
{
	printk(KERN_INFO "GEMINI: unloaded\n");
	return;
}

module_init(gemini_init);
module_exit(gemini_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR(AUTHOR);
MODULE_DESCRIPTION(DESC);
