#ifndef __LINUX_SYMS_H__
#define __LINUX_SYMS_H__

#include <linux/irq.h>

/* Linux symbols we have to link to ourselves */
extern int  (*linux_create_irq) (void);
extern void (*linux_destroy_irq)(unsigned int);
extern void (*linux_handle_edge_irq)(unsigned int, struct irq_desc *);
extern struct irq_desc * (*linux_irq_to_desc)(unsigned int);

int pisces_linux_symbol_init(void);

#endif
