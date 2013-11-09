#include <linux/kallsyms.h>

#include "linux_syms.h"

u64 * linux_trampoline_target;
struct mutex * linux_trampoline_lock;
pml4e64_t * linux_trampoline_pgd;
u64 linux_trampoline_startip;
u64 linux_start_secondary_addr;

unsigned long stack_start;

void (**linux_x86_platform_ipi_callback)(void);

/*
 * Init Linux symbols to interact with Linux trampoline
 */
void pisces_linux_symbol_init(void)
{
    struct real_mode_header * real_mode_header = NULL;
    struct trampoline_header * trampoline_header = NULL;

    /* u64 *linux_trampoline_target_ptr */
    real_mode_header = *(struct real_mode_header **) 
	kallsyms_lookup_name("real_mode_header");

    trampoline_header = (struct trampoline_header *) 
	__va(real_mode_header->trampoline_header);

    linux_trampoline_target = &trampoline_header->start;

    /* struct mutex *linux_trampoline_lock */
    linux_trampoline_lock = (struct mutex *) 
	kallsyms_lookup_name("cpu_add_remove_lock");

    /* pml4e64_t *linux_trampoline_pgd */
    linux_trampoline_pgd = (pml4e64_t *)
	__va(real_mode_header->trampoline_pgd);

    /* u64 *linux_trampoline_startip */
    linux_trampoline_startip = real_mode_header->trampoline_start;

    linux_x86_platform_ipi_callback = (void (**)(void)) 
	kallsyms_lookup_name("x86_platform_ipi_callback");

    linux_start_secondary_addr = 
	kallsyms_lookup_name("start_secondary");

    stack_start = *(unsigned long *)
	kallsyms_lookup_name("stack_start");

}
