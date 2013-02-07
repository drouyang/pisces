#ifndef _PGTABLES_64_H_
#define _PGTABLES_64_H_

#include<linux/module.h>

/* LONG MODE 64 bit PAGE STRUCTURES */
/* page size 2MB */

#define NUM_PGD_ENTRIES         512
#define NUM_PUD_ENTRIES         512
#define NUM_PMD_ENTRIES         512


/* Converts an address into a page table index */

#define PGD_INDEX(x) ((((u64)x) >> 39) & 0x1ff)
#define PUD_INDEX(x) ((((u64)x) >> 30) & 0x1ff)
#define PMD_INDEX(x) ((((u64)x) >> 21) & 0x1ff)


/* Gets the base address needed for a Page Table entry */
#define PAGE_TO_BASE_ADDR(x) (((uintptr_t)x) >> 12)
#define PAGE_TO_BASE_ADDR_2MB(x) (((uintptr_t)x) >> 21)

#define BASE_TO_PAGE_ADDR(x) (((uintptr_t)x) << 12)
#define BASE_TO_PAGE_ADDR_2MB(x) (((uintptr_t)x) << 21)


#define PAGE_OFFSET_2MB(x) ((x) & 0x1fffff)


// We shift instead of mask because we don't know the address size
#define PAGE_POWER_2MB 21
#define PAGE_ADDR(x) (((x) >> PAGE_POWER_2MB) << PAGE_POWER_2MB)

//#define PAGE_SIZE 4096
#define PAGE_SIZE_2MB (4096 * 512)

/* *** */




#define CR3_TO_PGD_PA(cr3)  ((uintptr_t)(((u64)cr3) & 0x000ffffffffff000LL))

#define CR3_TO_PGD_VA(cr3)  ((pgd_t *)__va((void *)(uintptr_t)(((u64)cr3) & 0x000ffffffffff000LL)))



typedef struct gen_pt {
    u32 present        : 1;
    u32 writable       : 1;
    u32 user_page      : 1;
} __attribute__((packed)) gen_pt_t;


//#define _KERNPG_TABLE (_PAGE_PRESENT | _PAGE_RW | _PAGE_ACCESSED | _PAGE_DIRTY)
typedef struct pgd_2MB {
    u64 present        : 1;
    u64 writable       : 1;
    u64 user_page      : 1;
    u64 write_through  : 1;
    u64 cache_disable  : 1;
    u64 accessed       : 1;
    u64 reserved       : 1;
    u64 zero           : 2;
    u64 vmm_info       : 3;
    u64 base_addr      : 40;
    u64 available      : 11;
    u64 no_execute     : 1;
} __attribute__((packed)) pgd_2MB_t;


typedef struct pud_2MB {
    u64 present        : 1;
    u64 writable       : 1;
    u64 user_page      : 1;
    u64 write_through  : 1;
    u64 cache_disable  : 1;
    u64 accessed       : 1;
    u64 avail          : 1;
    u64 large_page     : 1;
    u64 zero           : 1;
    u64 vmm_info       : 3;
    u64 base_addr      : 40;
    u64 available      : 11;
    u64 no_execute     : 1;
} __attribute__((packed)) pud_2MB_t;


typedef struct pmd_2MB {
    u64 present         : 1;
    u64 writable        : 1;
    u64 user_page       : 1;
    u64 write_through   : 1;
    u64 cache_disable   : 1;
    u64 accessed        : 1;
    u64 dirty           : 1;
    u64 large_page      : 1;
    u64 global_page     : 1;
    u64 vmm_info        : 3;
    u64 pat             : 1;
    u64 rsvd            : 8;
    u64 base_addr       : 31;
    u64 available       : 11;
    u64 no_execute      : 1;
} __attribute__((packed)) pmd_2MB_t;


typedef struct pf_error_code {
    u32 present           : 1; // if 0, fault due to page not present
    u32 write             : 1; // if 1, faulting access was a write
    u32 user              : 1; // if 1, faulting access was in user mode
    u32 rsvd_access       : 1; // if 1, fault from reading a 1 from a reserved field (?)
    u32 ifetch            : 1; // if 1, faulting access was an instr fetch (only with NX)
    u32 rsvd              : 27;
} __attribute__((packed)) pf_error_t;

// Steal some virtual address space from kernel

/* 128GB of virtual address space, that will hopefully be unused
   These are pulled from the user space memory region 
   Note that this could be a serious issue, because this range is not reserved 

   Based on my thorough investigation (running `cat /proc/self/maps` ~20 times), 
   this appears to sit between the heap and runtime libraries
*/
#define GEMINI_REGION_START 0x1000000000LL
#define GEMINI_REGION_END   0x3000000000LL



// Returns the current CR3 value
static inline uintptr_t get_cr3(void) {
    u64 cr3 = 0;

    __asm__ __volatile__ ("movq %%cr3, %0; "
        : "=q"(cr3)
        :
        );

    return (uintptr_t)cr3;
}


static inline void invlpg(uintptr_t page_addr) {
    printk("Invalidating Address %p\n", (void *)page_addr);
    __asm__ __volatile__ ("invlpg (%0); "
        : 
        :"r"(page_addr)
        : "memory"
        );
}

#endif
