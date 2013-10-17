#ifndef _BOOT_H_
#define _BOOT_H_

void pisces_linux_symbol_init(void);
void setup_linux_trampoline_pgd(u64 target_addr);
inline void setup_linux_trampoline_target(u64 target_addr);
inline void reset_cpu(int apicid);
int setup_boot_params(struct pisces_enclave * enclave);
int boot_enclave(struct pisces_enclave * enclave);

void trampoline_lock(void);
void trampoline_unlock(void);

#endif
