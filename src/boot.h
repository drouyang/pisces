#ifndef _BOOT_H_
#define _BOOT_H_

inline void reset_cpu(int apicid);
int setup_boot_params(struct pisces_enclave * enclave);
int boot_enclave(struct pisces_enclave * enclave);

void set_linux_trampoline(struct pisces_enclave * enclave);
void restore_linux_trampoline(struct pisces_enclave * enclave);

void trampoline_lock(void);
void trampoline_unlock(void);

#endif
