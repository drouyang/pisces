#include <linux/kallsyms.h>
//#include <asm/desc_defs.h>

#include "linux_syms.h"

void (**linux_x86_platform_ipi_callback)(void);
int * linux_first_system_vector;
gate_desc * linux_idt_table;

void (*linux_irq_enter)(void);
void (*linux_irq_exit)(void);
void (*linux_exit_idle)(void);

/*
 * Init Linux symbols to interact with Linux trampoline
 */
int 
pisces_linux_symbol_init(void)
{
    unsigned long symbol_addr = 0;


    /* Symbol:
     *  --  linux_x86_platform_ipi_callback
     */
    {
        symbol_addr = kallsyms_lookup_name("x86_platform_ipi_callback");
      
	if (symbol_addr == 0) {
            printk(KERN_WARNING "Linux symbol x86_platform_ipi_callback not found.\n");
            return -1;
        }

        linux_x86_platform_ipi_callback = (void (**)(void)) symbol_addr;
    }

    /* Symbol: 
     * --  linux_first_system_vector
     */
    {
	symbol_addr = kallsyms_lookup_name("first_system_vector");

	if (symbol_addr == 0) {
	    printk(KERN_WARNING "Linux symbol first_system_vector not found.\n");
	    return -1;
	}

	linux_first_system_vector = (int *) symbol_addr;
    }

    /* Symbol:
     * --  linux_idt_table
     */
    {
	symbol_addr = kallsyms_lookup_name("idt_table");

	if (symbol_addr == 0) {
	    printk(KERN_WARNING "Linux symbol idt_table not found.\n");
	    return -1;
	}

	linux_idt_table = (gate_desc *) symbol_addr;
    }

    /* Symbol:
     * --  linux_irq_enter
     */
    {
	symbol_addr = kallsyms_lookup_name("irq_enter");

	if (symbol_addr == 0) {
	    printk(KERN_WARNING "Linux symbol irq_enter not found.\n");
	    return -1;
	}

	linux_irq_enter = (void (*)(void))symbol_addr;
    } 

    /* Symbol:
     * --  linux_irq_exit
     */
    {
	symbol_addr = kallsyms_lookup_name("irq_exit");

	if (symbol_addr == 0) {
	    printk(KERN_WARNING "Linux symbol irq_exit not found.\n");
	    return -1;
	}

	linux_irq_exit = (void (*)(void))symbol_addr;
    } 

    /* Symbol:
     * --  linux_irq_enter
     */
    {
	symbol_addr = kallsyms_lookup_name("exit_idle");

	if (symbol_addr == 0) {
	    printk(KERN_WARNING "Linux symbol exit_idle not found.\n");
	    return -1;
	}

	linux_exit_idle = (void (*)(void))symbol_addr;
    } 

    return 0;
}
