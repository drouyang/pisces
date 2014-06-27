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

int 
init_linux_trampoline(void)
{

    struct page              * pgd_cache_page    = NULL;
    struct real_mode_header  * real_mode_header  = NULL;
    struct trampoline_header * trampoline_header = NULL;

    /* Symbols:
     *  --  trampoline_target
     *  --  trampoline_pgd
     *  --  cpu_init_ip
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
	
	
	trampoline_state.cpu_init_rip = real_mode_header->trampoline_start;
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
setup_linux_trampoline(struct pisces_enclave * enclave)
{
    //    struct pisces_boot_params * boot_params = (struct pisces_boot_params *)__va(enclave->bootmem_addr_pa);
 
    //    int index      = PML4E64_INDEX(enclave->bootmem_addr_pa);

    memcpy(cached_pgd_ptr, trampoline_pgd_ptr, PAGE_SIZE);       /* Save current trampoline PGD contents */

    /* Identity map bootmem region         */
    /* Kernel map bootmem region           */
  
    memcpy(trampoline_pgd_ptr, __va(trampoline_state.pml_pa), PAGE_SIZE);       /* Copy in our own PGD */



    cached_trampoline_target = *trampoline_target;
    *trampoline_target       = enclave->bootmem_addr_pa;

    printk(KERN_DEBUG "Set linux trampoline target: %p -> %p\n", 
            (void *) cached_trampoline_target, (void *)enclave->bootmem_addr_pa);


    return 0;
}






int
restore_linux_trampoline(struct pisces_enclave * enclave)
{
    //u64 target_addr = enclave->bootmem_addr_pa;
    //   u64 target_addr = 0;
    //    int index       = PML4E64_INDEX(target_addr);

    memcpy(trampoline_pgd_ptr, cached_pgd_ptr, PAGE_SIZE);

    /* Free enclave pgtables */


    printk("Restored Linux Trampoline\n");

    *trampoline_target = cached_trampoline_target;

    printk("Restored linux trampoline target: %p\n", 
	   (void *) *trampoline_target);

    return 0;
}














#if 0









// setup bootstrap page tables - pisces_ident_pgt
// 1G identity mapping start from 0
// and 1G mapping start from enclave->bootmem_addr_pa higher than 1G
#define MEM_ADDR_1G 0x40000000

static int 
setup_ident_pts(struct pisces_enclave     * enclave,
		struct pisces_boot_params * boot_params, 
		uintptr_t                   target_addr) 
{
    struct pisces_ident_pgt * ident_pgt = (struct pisces_ident_pgt *)target_addr;

    memset(ident_pgt, 0, sizeof(struct pisces_ident_pgt));

    boot_params->ident_pml4e64.pdp_base_addr = PAGE_TO_BASE_ADDR(__pa(ident_pgt->pdp));
    boot_params->ident_pml4e64.present       = 1;
    boot_params->ident_pml4e64.writable      = 1;
    boot_params->ident_pml4e64.accessed      = 1;

    printk(KERN_INFO "  ident_pml4e64 entry: %p", 
	   (void *) *(u64 *) &boot_params->ident_pml4e64);

    /* 1G ident mapping start from 0 for trampoline code
     * and enclave boot memory lower than 1G
     */

    {
        int index = 0;
        u64 addr  = 0;
        u64 i     = 0;

        index = PDPE64_INDEX(addr);

        ident_pgt->pdp[index].pd_base_addr = PAGE_TO_BASE_ADDR(__pa(ident_pgt->pd0));
        ident_pgt->pdp[index].present      = 1;
        ident_pgt->pdp[index].writable     = 1;
        ident_pgt->pdp[index].accessed     = 1;

        printk(KERN_INFO "  pdp[%d]: %p\n", 
	       index, 
                (void *) *(u64 *) &ident_pgt->pdp[index]);

        for (i = 0; i < MAX_PDE64_ENTRIES; i++) {

            index = PDE64_INDEX(addr);

            ident_pgt->pd0[index].page_base_addr  = PAGE_TO_BASE_ADDR_2MB(addr);
            ident_pgt->pd0[index].large_page      = 1;
            ident_pgt->pd0[index].present         = 1;
            ident_pgt->pd0[index].writable        = 1;
            ident_pgt->pd0[index].dirty           = 1;
            ident_pgt->pd0[index].accessed        = 1;
            ident_pgt->pd0[index].global_page     = 1;   /* <---- HACK! HACK! HACK! HACK! (HOLY SHIT!!!)

							    When Linux/Kitten boot secondary cores they jump out of the trampoline 
							    to kernel _VIRTUAL_ addresses. Pisces does not currently provide a mapping for 
							    those virtual addresses and so it sets the trampoline target to the  physical/identity 
							    mapped address. However Linux/Kitten blow away their identity mappings after the BSP 
							    comes up so the address pisces uses is invalid. The hideous trick hidden here is that 
							    by setting the global bit the TLB entries for the identity mapping won't be lost once the 
							    page tables are reloaded. So effectively pisces forces the enclave to execute the secondary
							    startup code from _STALE_ TLB entries. 

							    Pisces must just hope to god that the secondary startup doesn't span PTE entries and miss 
							    in the TLB, or that the enclave uses a more effective TLB flush mechanism, or is executing 
							    on a platform that does not honor the PGE flag (Like QEMU!!! or a number of other VMMs).
							    Otherwise the enclave is going to trigger a page fault with the IDT loaded at an unmapped address, 
							    leading to a double fault, leading to a hard system reset. 
							 
							    WHY THE FUCK WASN'T THIS COMMENTED????????
							 */


	    addr += PAGE_SIZE_2MB;
        }
    }

    /* 1G ident mapping start from bootmem_addr
     * for enclave loaded higher than 1G
     */
    if (enclave->bootmem_addr_pa  > MEM_ADDR_1G) {
        int index = 0;
        u64 addr  = enclave->bootmem_addr_pa;
        u64 i     = 0;

        index = PDPE64_INDEX(addr);

        ident_pgt->pdp[index].pd_base_addr  = PAGE_TO_BASE_ADDR(__pa(ident_pgt->pd1));
        ident_pgt->pdp[index].present       = 1;
        ident_pgt->pdp[index].writable      = 1;
        ident_pgt->pdp[index].accessed      = 1;

        printk(KERN_INFO "  pdp[%d]: %p\n", 
	       index, 
	       (void *) *(u64 *) &ident_pgt->pdp[index]);

        for (i = 0; i < MAX_PDE64_ENTRIES; i++) {

            index = PDE64_INDEX(addr);

            ident_pgt->pd1[index].page_base_addr  = PAGE_TO_BASE_ADDR_2MB(addr);
            ident_pgt->pd1[index].large_page      = 1;
            ident_pgt->pd1[index].present         = 1;
            ident_pgt->pd1[index].writable        = 1;
            ident_pgt->pd1[index].dirty           = 1;
            ident_pgt->pd1[index].accessed        = 1;
            ident_pgt->pd1[index].global_page     = 1;

	    addr += PAGE_SIZE_2MB;
        }
    }

    return 0;
}



#endif
