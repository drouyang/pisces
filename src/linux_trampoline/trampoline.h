/* 
 * Interface code to new Linux trampoline implementation
 *  -- Kernel versions > 3.0 or so
 * (c) 2014, Jack Lange <jacklange@cs.pitt.edu>
 */


#ifndef __LINUX_TRAMPOLINE_H__
#define __LINUX_TRAMPOLINE_H__


struct pisces_enclave;

int init_linux_trampoline(void);
int deinit_linux_trampoline(void);
int setup_linux_trampoline(struct pisces_enclave * enclave);
int restore_linux_trampoline(struct pisces_enclave * enclave);


#endif
