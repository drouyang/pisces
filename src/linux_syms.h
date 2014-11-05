#ifndef __LINUX_SYMS_H__
#define __LINUX_SYMS_H__

#include "pgtables.h"
#include <asm/desc_defs.h>

/* Linux symbols we have to link to ourselves */

extern void (**linux_x86_platform_ipi_callback)(void);
extern int * linux_first_system_vector;
extern gate_desc * linux_idt_table;

extern void (*linux_irq_enter)(void);
extern void (*linux_irq_exit)(void);
extern void (*linux_exit_idle)(void);

int pisces_linux_symbol_init(void);

#endif
