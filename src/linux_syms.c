#include <linux/kallsyms.h>

#include "linux_syms.h"

u64 * linux_trampoline_target;
struct mutex * linux_trampoline_lock;
pml4e64_t * linux_trampoline_pgd;
u64 linux_level3_ident_pgt_pa;
u64 linux_trampoline_startip;
u64 linux_start_secondary_addr;

unsigned long stack_start;

void (**linux_x86_platform_ipi_callback)(void);

/*
 * Init Linux symbols to interact with Linux trampoline
 */
int pisces_linux_symbol_init(void)
{
    struct real_mode_header * real_mode_header = NULL;
    struct trampoline_header * trampoline_header = NULL;
    unsigned long symbol_addr;

    {
        symbol_addr = kallsyms_lookup_name("real_mode_header");
        if (symbol_addr == 0) {
            printk(KERN_WARNING "Linux symbol real_mode_header not found.\n");
            return -1;
        }
        /* u64 *linux_trampoline_target_ptr */
        real_mode_header = *(struct real_mode_header **) symbol_addr;

        trampoline_header = (struct trampoline_header *) 
            __va(real_mode_header->trampoline_header);

        linux_trampoline_target = &trampoline_header->start;

        /* pml4e64_t *linux_trampoline_pgd */
        linux_trampoline_pgd = (pml4e64_t *)
            __va(real_mode_header->trampoline_pgd);

        /* u64 *linux_trampoline_startip */
        linux_trampoline_startip = real_mode_header->trampoline_start;
    }

    {
        /* struct mutex *linux_trampoline_lock */
        symbol_addr = kallsyms_lookup_name("cpu_add_remove_lock");
        if (symbol_addr == 0) {
            printk(KERN_WARNING "Linux symbol cpu_add_remove_lock not found.\n");
            return -1;
        }
        linux_trampoline_lock = (struct mutex *) symbol_addr;
    }

    //{
    //    symbol_addr = kallsyms_lookup_name("level3_ident_pgt");
    //    if (symbol_addr == 0) {
    //        printk(KERN_WARNING "Linux symbol level3_ident_pgt not found.\n");
    //        return -1;
    //    }
    //    linux_level3_ident_pgt_pa = *(u64 *) symbol_addr;
    //}

    {
        symbol_addr = kallsyms_lookup_name("x86_platform_ipi_callback");
        if (symbol_addr == 0) {
            printk(KERN_WARNING "Linux symbol x86_platform_ipi_callback not found.\n");
            return -1;
        }
        linux_x86_platform_ipi_callback = (void (**)(void)) symbol_addr;
    }

    {
        symbol_addr = kallsyms_lookup_name("start_secondary");
        if (symbol_addr == 0) {
            printk(KERN_WARNING "Linux symbol start_secondary not found.\n");
            return -1;
        }
        linux_start_secondary_addr = symbol_addr;
    }

    {
        symbol_addr = kallsyms_lookup_name("stack_start");
        if (symbol_addr == 0) {
            printk(KERN_WARNING "Linux symbol stack_start not found.\n");
            return -1;
        }
        stack_start = *(unsigned long *) symbol_addr;
    }
    return 0;
}
