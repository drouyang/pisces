#ifndef __LINUX_SYMS_H__
#define __LINUX_SYMS_H__

#include "pgtables.h"

/* Linux symbols we have to link to ourselves */
extern u64 *linux_trampoline_target;
extern struct mutex *linux_trampoline_lock;
extern pml4e64_t *linux_trampoline_pgd;
extern u64 linux_trampoline_startip;

extern void (**linux_x86_platform_ipi_callback)(void);
extern u64 linux_start_secondary_addr;


extern unsigned long stack_start; 
/* ** */

int pisces_linux_symbol_init(void);

#endif
