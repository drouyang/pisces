#include <linux/kallsyms.h>
#include <linux/version.h>
#include "linux_syms.h"

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,16,0)
int  (*linux_create_irq) (void);
void (*linux_destroy_irq)(unsigned int);
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,4,0)
void (*linux_handle_edge_irq)(unsigned int, struct irq_desc *);
struct irq_desc * (*linux_irq_to_desc)(unsigned int);
#else
#define linux_handle_edge_irq handle_edge_irq
#define linux_irq_to_desc irq_to_desc
#endif

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

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,4,0)
    /* Symbol:
     *	--  handle_edge_irq
     */
    {
	symbol_addr = kallsyms_lookup_name("handle_edge_irq");
      
	if (symbol_addr == 0) {
	    printk(KERN_WARNING "Linux symbol handle_edge_irq not found.\n");
	    return -1;
	}

	linux_handle_edge_irq = (void (*)(unsigned int, struct irq_desc *))symbol_addr;
    }

    /* Symbol:
     *	--  irq_to_desc
     */
    {
	symbol_addr = kallsyms_lookup_name("irq_to_desc");
      
	if (symbol_addr == 0) {
	    printk(KERN_WARNING "Linux symbol irq_to_desc not found.\n");
	    return -1;
	}

	linux_irq_to_desc = (struct irq_desc * (*)(unsigned int))symbol_addr;
    }
#endif

    return 0;
}
