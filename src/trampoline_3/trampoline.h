/* 
 * Interface code to new Linux trampoline implementation
 *  -- Kernel versions > 3.4 or so
 * (c) 2014, Jack Lange <jacklange@cs.pitt.edu>
 */


#ifndef __TRAMPOLINE_3_H__
#define __TRAMPOLINE_3_H__


struct pisces_enclave;

int init_trampoline(void);
int deinit_trampoline(void);
int setup_trampoline(struct pisces_enclave * enclave);
int restore_trampoline(struct pisces_enclave * enclave);


#endif
