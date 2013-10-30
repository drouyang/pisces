#ifndef __LINUX_IPI_H__
#define __LINUX_IPI_H__

struct pisces_enclave;

#define LINUX_STUPID_IPI 247

int pisces_register_ipi_callback(void (*callback)(void *), void * private_data);
int pisces_ipi_init(void);

int pisces_send_ipi(struct pisces_enclave * enclave, int cpu_id, unsigned int vector);

#endif
