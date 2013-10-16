#ifndef _BOOT_H_
#define _BOOT_H_

void pisces_linux_symbol_init(void);
void set_enclave_trampoline(struct pisces_enclave *enclave, u64 target_addr, u64 esi);
int setup_boot_params(struct pisces_enclave * enclave);
int boot_enclave(struct pisces_enclave * enclave);
void cpu_hot_add_reset(struct pisces_enclave * enclave, int apicid);

#endif
