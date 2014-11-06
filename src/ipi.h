#ifndef __LINUX_IPI_H__
#define __LINUX_IPI_H__

#include <asm/irq_vectors.h>

#define PISCES_LCALL_VECTOR X86_PLATFORM_IPI_VECTOR

struct pisces_enclave;


int pisces_request_ipi_vector(void (*callback)(unsigned int, void *), void * private_data);
int pisces_release_ipi_vector(int vector);

int pisces_ipi_init(void);
int pisces_ipi_deinit(void);

void pisces_send_enclave_ipi(struct pisces_enclave * enclave, unsigned int vector);
void pisces_send_ipi(unsigned int vector, unsigned int cpu);

#endif
