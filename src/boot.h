#ifndef _BOOT_H_
#define _BOOT_H_

#include "pgtables.h"

struct trampoline_data {
    uintptr_t pml_pa;

    uintptr_t pdp0_pa;   /* < 1GB identity map (trampoline) */
    uintptr_t pdp1_pa;   /* > 1GB identity map (bootmem)    */
    uintptr_t pdp2_pa;   /* Kernel address map (bootmem)    */   

    uintptr_t pd0_pa;     /* < 1GB identity map (trampoline) */
    uintptr_t pd1_pa;     /* > 1GB identity map (bootmem)    */
    uintptr_t pd2_pa;     /* Kernel address map (bootmem)    */  

    unsigned long cpu_init_rip;  
} __attribute__((aligned(PAGE_SIZE))) __attribute__((packed));

extern struct trampoline_data trampoline_state;

int boot_enclave(struct pisces_enclave * enclave);
int stop_enclave(struct pisces_enclave * enclave);


int pisces_init_trampoline(void);
int pisces_deinit_trampoline(void);

int pisces_setup_trampoline(struct pisces_enclave * enclave);
int pisces_restore_trampoline(struct pisces_enclave * enclave);


#endif
