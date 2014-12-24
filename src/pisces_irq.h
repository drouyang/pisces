#ifndef __PISCES_IRQ_H__
#define __PISCES_IRQ_H__

#include <linux/interrupt.h>

struct pisces_enclave;

int
pisces_request_irq(irqreturn_t (*cb)(int, void *),
                   void      * priv);

int
pisces_release_irq(int    irq,
                   void * priv);

int
pisces_irq_to_vector(int irq);

void
pisces_send_enclave_ipi(struct pisces_enclave * enclave,
                        unsigned int            vector);

#endif
