#include <linux/kallsyms.h>
#include <linux/version.h>
#include "linux_syms.h"

int  (*linux_create_irq) (void);
void (*linux_destroy_irq)(unsigned int);

/*
 * Init Linux symbols to interact with Linux trampoline
 */
int 
pisces_linux_symbol_init(void)
{
    unsigned long symbol_addr = 0;

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,16,0)
    /* Symbol:
     *	--  create_irq
     */
    {
	symbol_addr = kallsyms_lookup_name("create_irq");
      
	if (symbol_addr == 0) {
	    printk(KERN_WARNING "Linux symbol create_irq not found.\n");
	    return -1;
	}

	linux_create_irq = (int (*)(void))symbol_addr;
    }

    /* Symbol:
     *	--  destroy_irq
     */
    {
	symbol_addr = kallsyms_lookup_name("destroy_irq");
      
	if (symbol_addr == 0) {
	    printk(KERN_WARNING "Linux symbol destroy_irq not found.\n");
	    return -1;
	}

	linux_destroy_irq = (void (*)(unsigned int))symbol_addr;
    }
#endif

    return 0;
}
