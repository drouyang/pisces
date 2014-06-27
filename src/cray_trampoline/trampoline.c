/* 
 * Interface to setup trampoline for Gemini era Cray Linux 
 *  (c) 2014, Jack Lange <jacklange@cs.pitt.edu>
 */

#include <linux/init.h>
#include <asm/io.h>
#include <asm/trampoline.h>


#include "../enclave.h"
#include "../pgtables.h"
#include "../boot.h"
#include "trampoline.h"



extern u8  trampoline_level4_pgt[];
extern u64 secondary_startup_vector;



int 
init_cray_trampoline(void)
{
    trampoline_state.cpu_init_rip = TRAMPOLINE_BASE;

    return 0;
}


int
setup_cray_trampoline(struct pisces_enclave * enclave)
{
    //    struct pisces_boot_params * boot_params = (struct pisces_boot_params *)__va(enclave->bootmem_addr_pa);

    printk("Setting up Cray Trampoline\n");


    /* Setup Trampoline Code */    
    
    secondary_startup_vector = enclave->bootmem_addr_pa;                     /* Target RIP */

    memset(trampoline_level4_pgt, 0, PAGE_SIZE);                             /* Clear old PT Entries */

    memcpy(trampoline_level4_pgt, __va(trampoline_state.pml_pa), PAGE_SIZE); /* Overwrite 1st PML Entry (512GB) with our own PDP */
    
    memcpy(__va(TRAMPOLINE_BASE), trampoline_data, TRAMPOLINE_SIZE);


    
    printk("Final startup_target = %p\n", 
	   (void *)*(u64 *)((u64)__va(TRAMPOLINE_BASE) + ((u64)&secondary_startup_vector - (u64)trampoline_data)));

    return 0;
}


int
restore_cray_trampoline(struct pisces_enclave * enclave)
{

    /* Do we need to do anything here??? */
    return 0;

}

