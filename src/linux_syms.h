#ifndef __LINUX_SYMS_H__
#define __LINUX_SYMS_H__

#include "pgtables.h"

/* Linux symbols we have to link to ourselves */

extern void (**linux_x86_platform_ipi_callback)(void);
/* ** */

int pisces_linux_symbol_init(void);

#endif
