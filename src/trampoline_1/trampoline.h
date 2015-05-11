/* 
 * Interface to setup trampoline for Gemini era Cray Linux 
 *  (c) 2014, Jack Lange <jacklange@cs.pitt.edu>
 */
#ifndef __TRAMPOLINE_1__
#define __TRAMPOLINE_1__

struct pisces_enclave;

int init_trampoline(void);
int deinit_trampoline(void);
int setup_trampoline(struct pisces_enclave * enclave);
int restore_trampoline(struct pisces_enclave * enclave);




#endif
