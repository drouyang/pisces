#ifndef __LINUX_SYMS_H__
#define __LINUX_SYMS_H__

/* Linux symbols we have to link to ourselves */
extern int  (*linux_create_irq) (void);
extern void (*linux_destroy_irq)(unsigned int);

int pisces_linux_symbol_init(void);

#endif
