#ifndef __LINUX_SYMS_H__
#define __LINUX_SYMS_H__

#include <linux/irq.h>

/* Linux symbols we have to link to ourselves */
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,16,0)
extern int  (*linux_create_irq) (void);
extern void (*linux_destroy_irq)(unsigned int);
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,4,0)
extern void (*linux_handle_edge_irq)(unsigned int, struct irq_desc *);
extern struct irq_desc * (*linux_irq_to_desc)(unsigned int);
#else
#define linux_handle_edge_irq handle_edge_irq
#define linux_irq_to_desc irq_to_desc
#endif


int pisces_linux_symbol_init(void);

#endif
