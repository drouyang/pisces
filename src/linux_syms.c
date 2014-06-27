#include <linux/kallsyms.h>

#include "linux_syms.h"



struct mutex * linux_trampoline_lock;


void (**linux_x86_platform_ipi_callback)(void);

/*
 * Init Linux symbols to interact with Linux trampoline
 */
int 
pisces_linux_symbol_init(void)
{
    unsigned long symbol_addr = 0;


    /* Symbol:
     *  --  linux_trampoline_lock
     */
    {
        symbol_addr = kallsyms_lookup_name("cpu_add_remove_lock");

        if (symbol_addr == 0) {
            printk(KERN_WARNING "Linux symbol cpu_add_remove_lock not found.\n");
            return -1;
        }

        linux_trampoline_lock = (struct mutex *)symbol_addr;
    }

    
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




    return 0;
}
