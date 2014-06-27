#include <linux/types.h>
#include "pgtables.h"


// Returns the current CR3 value
static inline uintptr_t get_cr3(void) {
    u64 cr3 = 0;

    __asm__ __volatile__ ("movq %%cr3, %0; "
            : "=q"(cr3)
            :
            );

    return (uintptr_t)cr3;
}

int 
dump_pgtables(uintptr_t pgt_ptr, 
	      uintptr_t vaddr) 
{
    pml4e64_t * pml = (pml4e64_t *) pgt_ptr;
    pdpe64_t  * pdp = NULL;
    pde64_t   * pd  = NULL;
    pte64_t   * pt  = NULL;

    pml4e64_t * pml_entry = NULL;
    pdpe64_t  * pdp_entry = NULL;
    pde64_t   * pd_entry  = NULL;
    pte64_t   * pt_entry  = NULL;

    if (pgt_ptr == 0) {
        printk("Error dumpping a NULL cr3\n");
        return -1;
    }

    printk("Dumpping page VA=%p (pml=%p)\n", (void *)vaddr, pml);

    pml_entry = &pml[PML4E64_INDEX(vaddr)];

    if (!pml_entry->present) {
        printk("PDP Not present (idx = %llu)\n", PML4E64_INDEX(vaddr));
        return -1;
    } else {
        printk("Found PDP (idx = %llu)\n", PML4E64_INDEX(vaddr));
        pdp = __va(BASE_TO_PAGE_ADDR(pml_entry->pdp_base_addr));
    }

    pdp_entry = &pdp[PDPE64_INDEX(vaddr)];

    if (!pdp_entry->present) {
        printk("PD not present (idx = %llu)\n", PDPE64_INDEX(vaddr));
        return -1;
    } else if (pdp_entry->large_page) {
        printk("1G large page at %p\n", (void *) BASE_TO_PAGE_ADDR_1GB(((pdpe64_1GB_t *)pdp_entry)->page_base_addr));
        return 0;
    } else {
        printk("Found PD (idx = %llu)\n", PDPE64_INDEX(vaddr));
        pd = __va(BASE_TO_PAGE_ADDR(pdp_entry->pd_base_addr));
    }

    pd_entry = &pd[PDE64_INDEX(vaddr)];
    if (!pd_entry->present) {
        printk("PT not present (idx = %llu)\n", PDE64_INDEX(vaddr));
        return -1;
    } else if (pd_entry->large_page) {
        printk("2M large page at %p\n", (void *) (BASE_TO_PAGE_ADDR_2MB(((pde64_2MB_t *)pd_entry)->page_base_addr)));
        return 0;
    } else {
        printk("Found PT (idx = %llu)\n", PDE64_INDEX(vaddr));
        pt = __va(BASE_TO_PAGE_ADDR(pd_entry->pt_base_addr));
    }

    pt_entry = &pt[PTE64_INDEX(vaddr)];

    if (!pt_entry->present) {
        printk("Page entry not present (idx=%llu), vaddr = %p\n", PTE64_INDEX(vaddr), (void *) vaddr);
        return -1;
    } else {
        uintptr_t page_addr = BASE_TO_PAGE_ADDR(pt_entry->page_base_addr);

        printk("Page addr: %p\n", (void *) page_addr);
    }

    return 0;
}



int 
walk_pgtables(uintptr_t pgt_ptr) 
{
    pml4e64_t * pml = (pml4e64_t *)pgt_ptr;
    pdpe64_t  * pdp = NULL;
    pde64_t   * pd  = NULL;
    pte64_t   * pt  = NULL;

    pml4e64_t * pml_entry = NULL;
    pdpe64_t  * pdp_entry = NULL;
    pde64_t   * pd_entry  = NULL;
    pte64_t   * pt_entry  = NULL;   

    int i = 0;
    int j = 0;
    int k = 0;
    int m = 0;

    uintptr_t addr_iter = 0;

    if (pgt_ptr == 0) {
        printk("Error dumpping a NULL cr3\n");
        return -1;
    }

    
    for (i = 0; i < 512; i++) {
	
	pml_entry = &pml[i];

	if (!pml_entry->present) {
	    // printk("PDP Not present (idx = %llu)\n", PML4E64_INDEX(vaddr));
	    addr_iter += (uintptr_t)(PAGE_SIZE_1GB) * (uintptr_t)512;
	    continue;
	} else {
	    // printk("Found PDP (idx = %llu)\n", PML4E64_INDEX(vaddr));
	    pdp = __va(BASE_TO_PAGE_ADDR(pml_entry->pdp_base_addr));
	}

	for (j = 0; j < 512; j++) {


	    pdp_entry = &pdp[j];
	    
	    if (!pdp_entry->present) {
		addr_iter += PAGE_SIZE_1GB;
		continue;
	    } else if (pdp_entry->large_page) {
		printk("1G page: %p -> %p\n", 
		       (void *)addr_iter, 
		       (void *) BASE_TO_PAGE_ADDR_1GB(((pdpe64_1GB_t *)pdp_entry)->page_base_addr));
		addr_iter += PAGE_SIZE_1GB;
		continue;
	    } else {
		//		printk("Found PD (idx = %llu)\n", PDPE64_INDEX(vaddr));
		pd = __va(BASE_TO_PAGE_ADDR(pdp_entry->pd_base_addr));
	    }


	    for (k = 0; k < 512; k++) {

		pd_entry = &pd[k];

		if (!pd_entry->present) {
		    //		    printk("PT not present (idx = %llu)\n", PDE64_INDEX(vaddr));
		    addr_iter += PAGE_SIZE_2MB;
		    continue;
		    
		} else if (pd_entry->large_page) {
		    printk("2M page: %p -> %p (i=%d, j=%d, k=%d, m=%d)\n", 
			   (void *)addr_iter,
			   (void *) (BASE_TO_PAGE_ADDR_2MB(((pde64_2MB_t *)pd_entry)->page_base_addr)),
			   i, j, k, m);

		    addr_iter += PAGE_SIZE_2MB;

		    continue;
		} else {
		    //		    printk("Found PT (idx = %llu)\n", PDE64_INDEX(vaddr));
		    pt = __va(BASE_TO_PAGE_ADDR(pd_entry->pt_base_addr));
		}



		for (m = 0; m < 512; m++) {


		    pt_entry = &pt[m];
		    
		    if (!pt_entry->present) {
			//	printk("Page entry not present (idx=%llu), vaddr = %p\n", PTE64_INDEX(vaddr), (void *) vaddr);
			addr_iter += PAGE_SIZE_4KB;
		    } else {
			uintptr_t page_addr = BASE_TO_PAGE_ADDR(pt_entry->page_base_addr);
			
			printk("4K page: %p -> %p\n", 
			       (void *)addr_iter,
			       (void *) page_addr);

			addr_iter += PAGE_SIZE_4KB;
		    }

		}


	    }

	}
	

    }

    return 0;

}


