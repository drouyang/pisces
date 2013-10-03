#ifndef _PISCES_XCALL_H_
#define _PISCES_XCALL_H_

struct pisces_enclave;

int pisces_send_ipi(struct pisces_enclave * enclave, int cpu_id, unsigned int vector);

#endif
