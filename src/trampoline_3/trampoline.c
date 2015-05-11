/* 
 * Interface code to new Linux trampoline implementation
 *  -- Kernel versions > 3.0 or so
 * (c) 2014, Jack Lange <jacklange@cs.pitt.edu>
 */

#include <linux/types.h>

#include "../pgtables.h"
#include "../enclave.h"
#include "../pisces_boot_params.h"
#include "../boot.h"

#include "trampoline.h"


#include <linux/kallsyms.h>

static u64 * trampoline_target  = NULL;
static u8  * trampoline_pgd_ptr = NULL;


static u8  * cached_pgd_ptr           = NULL;
static u64   cached_trampoline_target = 0;

void (*__cpu_maps_update_begin)(void);
void (*__cpu_maps_update_done)(void);


int 
init_trampoline(void)
{

    struct page              * pgd_cache_page    = NULL;
    struct real_mode_header  * real_mode_header  = NULL;
    struct trampoline_header * trampoline_header = NULL;
    unsigned long              symbol_addr       = 0;

    /* Symbols:
     *  --  trampoline_target
     *  --  trampoline_pgd
     */
    {
	unsigned long symbol_addr = 0;

	symbol_addr = kallsyms_lookup_name("real_mode_header");
	
	if (symbol_addr == 0) {
	    printk(KERN_WARNING "Linux symbol real_mode_header not found.\n");
	    return -1;
	}
	
	real_mode_header         = *(struct real_mode_header **)symbol_addr;
	trampoline_header        =  (struct trampoline_header *)__va(real_mode_header->trampoline_header);
	
	/* u64 *linux_trampoline_target_ptr */
	trampoline_target  = &trampoline_header->start;
	
	/* u8 * linux_trampoline_pgd */
	trampoline_pgd_ptr = (u8 *)__va(real_mode_header->trampoline_pgd);
	

	/* Record the location of the trampoline to send with the INIT/SIPI IPI */
	trampoline_state.cpu_init_rip = real_mode_header->trampoline_start;
    }
    
    /* Symbol:
     *  --  cpu_maps_update_begin
     */
    {
        symbol_addr = kallsyms_lookup_name("cpu_maps_update_begin");

        if (symbol_addr == 0) {
            printk(KERN_WARNING "Linux symbol cpu_add_remove_lock not found.\n");
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
            printk(KERN_WARNING "Linux symbol cpu_add_remove_lock not found.\n");
            return -1;
        }

        __cpu_maps_update_done = (void (*)(void))symbol_addr;
    }




    pgd_cache_page = alloc_page(GFP_KERNEL);

    if (pgd_cache_page == NULL) {
	printk(KERN_ERR "Error: could not allocate page for caching original trampoline PGD\n");
	return -1;
    }

    cached_pgd_ptr = __va(page_to_pfn(pgd_cache_page) << PAGE_SHIFT);

    memset(cached_pgd_ptr, 0, PAGE_SIZE);
    
    return 0;
}


int 
deinit_trampoline(void) 
{
    free_page((uintptr_t)cached_pgd_ptr);

    return 0;
}




int 
setup_trampoline(struct pisces_enclave * enclave)
{
 
 
    __cpu_maps_update_begin();    /* Acquire mutex in Linux to prevent CPU operations */
    
    memcpy(cached_pgd_ptr, trampoline_pgd_ptr, PAGE_SIZE);       /* Save current trampoline PGD contents */

  
    memcpy(trampoline_pgd_ptr, __va(trampoline_state.pml_pa), PAGE_SIZE);       /* Copy in our own PGD */


    cached_trampoline_target = *trampoline_target;
    *trampoline_target       = enclave->bootmem_addr_pa;

    printk(KERN_DEBUG "Set linux trampoline target: %p -> %p\n", 
            (void *) cached_trampoline_target, (void *)enclave->bootmem_addr_pa);


    return 0;
}






int
restore_trampoline(struct pisces_enclave * enclave)
{

    memcpy(trampoline_pgd_ptr, cached_pgd_ptr, PAGE_SIZE);
    *trampoline_target = cached_trampoline_target;

    
    __cpu_maps_update_done();   /* Relase Linux Mutex for CPU operations */

    printk("Restored linux trampoline target: %p\n", 
	   (void *) *trampoline_target);

    return 0;
}


