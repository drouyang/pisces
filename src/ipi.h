#ifndef __LINUX_IPI_H__
#define __LINUX_IPI_H__

#include <asm/irq_vectors.h>

#define PISCES_LCALL_VECTOR X86_PLATFORM_IPI_VECTOR

struct pisces_enclave;


int pisces_register_ipi_callback(void (*callback)(void *), void * private_data);
int pisces_remove_ipi_callback(void (*callback)(void *), void * private_data);

int pisces_ipi_init(void);

int pisces_send_ipi(struct pisces_enclave * enclave, int cpu_id, unsigned int vector);

#endif
