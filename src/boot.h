#ifndef _BOOT_H_
#define _BOOT_H_

void pisces_linux_symbol_init(void);
int setup_boot_params(struct pisces_enclave * enclave);
int boot_enclave(struct pisces_enclave * enclave);
void cpu_hot_add(struct pisces_enclave * enclave, int apicid);

#endif
