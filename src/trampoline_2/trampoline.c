/* 
 * Interface to setup trampoline for 2.6.39 - 3.5
 *  (c) 2014, Jack Lange <jacklange@cs.pitt.edu>
 */

#include <linux/init.h>
#include <asm/io.h>
#include <asm/trampoline.h>
#include <linux/kallsyms.h>


#include "../enclave.h"
#include "../pgtables.h"
#include "../boot.h"
#include "trampoline.h"


#define TRAMPOLINE_SIZE roundup(trampoline_end - trampoline_data, PAGE_SIZE)

void (*__cpu_maps_update_begin)(void);
void (*__cpu_maps_update_done)(void);

unsigned char * __x86_trampoline_base = NULL;

unsigned char * cached_trampoline     = NULL;


int 
init_trampoline(void)
{

    unsigned long symbol_addr = 0;


    /* Symbol:
     *  --  cpu_maps_update_begin
     */
    {
        symbol_addr = kallsyms_lookup_name("cpu_maps_update_begin");

        if (symbol_addr == 0) {
            printk(KERN_WARNING "Linux symbol 'cpu_add_remove_lock' not found.\n");
            return -1;
        }

        __cpu_maps_update_begin = (void (*)(void))symbol_addr;
    }

    /* Symbol:
     *  --  cpu_maps_update_done
     */
    {
        symbol_addr = kallsyms_lookup_name("cpu_maps_update_done");

        if (symbol_addr == 0) {
            printk(KERN_WARNING "Linux symbol 'cpu_add_remove_lock' not found.\n");
            return -1;
        }

        __cpu_maps_update_done = (void (*)(void))symbol_addr;
    }


    /* Symbol:
     *  -- x86_trampoline_base
     */
    {
        symbol_addr = kallsyms_lookup_name("x86_trampoline_base");

        if (symbol_addr == 0) {
            printk(KERN_WARNING "Linux symbol 'x86_trampoline_base' not found.\n");
            return -1;
        }

       __x86_trampoline_base = (unsigned char *)symbol_addr
    }

    cached_trampoline = kmalloc(TRAMPOLINE_SIZE, GFP_KERNEL);

    /* Record the location of the trampoline to send with the INIT/SIPI IPI */
    trampoline_state.cpu_init_rip = __x86_trampoline_base;

    return 0;
}

int 
deinit_trampoline(void)
{  
    /* Nothing to do for now */

    return 0;
}


int
setup_trampoline(struct pisces_enclave * enclave)
{
    extern u8  trampoline_level4_pgt[];
    extern u64 secondary_startup_vector;
    
    printk("Setting up Cray Trampoline\n");


    __cpu_maps_update_begin();    /* Acquire mutex in Linux to prevent CPU operations */


    /* Cache old trampoline */
    memcpy(cached_trampoline, __va(__x86_trampoline_base), TRAMPOLINE_SIZE);

    /* Setup Trampoline Code */    
    
    secondary_startup_vector = enclave->bootmem_addr_pa;                     /* Target RIP */

    memset(trampoline_level4_pgt, 0, PAGE_SIZE);                             /* Clear old PT Entries */

    memcpy(trampoline_level4_pgt, __va(trampoline_state.pml_pa), PAGE_SIZE); /* Overwrite 1st PML Entry (512GB) with our own PDP */
    
    memcpy(__va(__x86_trampoline_base), trampoline_data, TRAMPOLINE_SIZE);


    
    printk("Final startup_target = %p\n", 
	   (void *)*(u64 *)((u64)__va(__x86_trampoline_base) + ((u64)&secondary_startup_vector - (u64)trampoline_data)));

    return 0;
}


int
restore_trampoline(struct pisces_enclave * enclave)
{

    /* Restore old trampoline */
    memcpy(__va(__x86_trampoline_base), cached_trampoline, TRAMPOLINE_SIZE);

    __cpu_maps_update_done();   /* Relase Linux Mutex for CPU operations */

    return 0;
}
