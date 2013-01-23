
void gemini_print(void)
{
    printk(KERN_INFO "GEMINI: IPI success!\n");
}
void printk_ipi(void)
{
        preempt_disable();
        ret = smp_call_function_single(1, gemini_print, NULL, 0);
        //apic->send_IPI_mask(cpumask_of(1), CALL_FUNCTION_SINGLE_VECTOR);
        preempt_enable();
	return;
}
