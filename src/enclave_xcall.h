#ifndef _ENCLAVE_XCALL_H_
#define _ENCLAVE_XCALL_H_

/* Same vector as X86_PLATFORM_IPI_VECTOR in Linux*/
#define PISCES_ENCLAVE_XCALL_VECTOR 247

int enclave_xcall_init(void);
void send_enclave_xcall(int apicid);

#endif
