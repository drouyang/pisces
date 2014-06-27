/* 
 * Interface to setup trampoline for Gemini era Cray Linux 
 *  (c) 2014, Jack Lange <jacklange@cs.pitt.edu>
 */
#ifndef __CRAY_TRAMPOLINE__
#define __CRAY_TRAMPOLINE__

struct pisces_enclave;

int init_cray_trampoline(void);
int deinit_cray_trampoline(void);
int setup_cray_trampoline(struct pisces_enclave * enclave);
int restore_cray_trampoline(struct pisces_enclave * enclave);




#endif
