#include <linux/kallsyms.h>

#include "linux_syms.h"


void (**linux_x86_platform_ipi_callback)(void);

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




    return 0;
}
