/* 
 * Interface to setup trampoline for 2.6.39 - 3.5
 *  (c) 2014, Jack Lange <jacklange@cs.pitt.edu>
 */
#ifndef __TRAMPOLINE_H__
#define __TRAMPOLINE_H__

struct pisces_enclave;

int init_trampoline(void);
int deinit_trampoline(void);
int setup_trampoline(struct pisces_enclave * enclave);
int restore_trampoline(struct pisces_enclave * enclave);




#endif
