/* 
 * Interface to setup trampoline for Gemini era Cray Linux (pre 2.6.33)
 *  (c) 2014, Jack Lange <jacklange@cs.pitt.edu>
 */
#ifndef __TRAMPOLINE_0__
#define __TRAMPOLINE_0__

struct pisces_enclave;

int init_trampoline(void);
int deinit_trampoline(void);
int setup_trampoline(struct pisces_enclave * enclave);
int restore_trampoline(struct pisces_enclave * enclave);




#endif
