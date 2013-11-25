/* Pisces page table management
 * (c) 2013, Brian Kocoloski (briankoco@cs.pitt.edu)
 */

/* We borrow heavily (read: directly) from XPMEM kernel code
 * (http://code.google.com/p/xpmem/)
 */

#include "pagemap.h"
#include <linux/hugetlb.h>

//static inline pte_t *
pte_t *
huge_pte_offset(struct mm_struct *mm, unsigned long addr)
{
    pgd_t *pgd;
    pud_t *pud;
    pmd_t *pmd = NULL;

    pgd = pgd_offset(mm, addr);
    if (pgd_present(*pgd)) {
        pud = pud_offset(pgd, addr);
        if (pud_present(*pud)) {
//            if (pud_huge(*pud))
//                return (pte_t *)pud;
            pmd = pmd_offset(pud, addr);
        }
    }
    return (pte_t *) pmd;
}


static inline pte_t *
xpmem_hugetlb_pte(struct mm_struct * mm, u64 vaddr, u64 * offset) {
    struct vm_area_struct * vma;
    u64 address;
    pte_t * pte;

    vma = find_vma(mm, vaddr);
    if (!vma)
        return NULL;

    if (is_vm_hugetlb_page(vma)) {
        struct hstate * hs = hstate_vma(vma);

        address = vaddr & huge_page_mask(hs);
        if (offset) {
            *offset = (vaddr & (huge_page_size(hs) - 1)) & PAGE_MASK;
        }

        pte = huge_pte_offset(mm, address);
        if (!pte || pte_none(*pte))
            return NULL;

        return (pte_t *)pte;
    }

    /*
     * We should never enter this area since xpmem_hugetlb_pte() is only called
     * if {pgd,pud,pmd}_large() is true
     */

    printk(KERN_ERR "Error - xpmem_hugetlb_pte incorrectly invoked\n");
    return NULL;
}

/*
 * Given an address space and a virtual address return a pointer to its 
 * pte if one is present.
 */
static inline pte_t *
xpmem_vaddr_to_pte_offset(struct mm_struct * mm, u64 vaddr, u64 * offset) {
    pgd_t * pgd;
    pud_t * pud;
    pmd_t * pmd;
    pte_t * pte;

    if (offset)
        *offset = 0;

    pgd = pgd_offset(mm, vaddr);
    if (!pgd_present(*pgd))
        return NULL;
    else if (pgd_large(*pgd)) {
        printk("pgd = %p\n", pgd);
        return xpmem_hugetlb_pte(mm, vaddr, offset);
    }

    pud = pud_offset(pgd, vaddr);
    if (!pud_present(*pud))
        return NULL;
    else if (pud_large(*pud)) {
        printk("pud = %p\n", pud);
        return xpmem_hugetlb_pte(mm, vaddr, offset);
    }    

    pmd = pmd_offset(pud, vaddr);
    if (!pmd_present(*pmd))
        return NULL;
    else if (pmd_large(*pmd)) {
        printk("pmd = %p\n", pmd);
        return xpmem_hugetlb_pte(mm, vaddr, offset);
    }    

    pte = pte_offset_map(pmd, vaddr);
    if (!pte_present(*pte))
        return NULL;

    return pte;
}

static inline u64
xpmem_vaddr_to_PFN(struct mm_struct * mm, u64 vaddr) {
    pte_t * pte;
    u64 pfn, offset;

    pte = xpmem_vaddr_to_pte_offset(mm, vaddr, &offset);
    if (pte == NULL)
        return 0;

    pfn = pte_pfn(*pte) + (offset >> PAGE_SHIFT);

    return pfn;
}


/* Set the page map for the PFN range in xpmem_addr's source address space */
int pisces_get_xpmem_map(
            struct pisces_xpmem_make * src_addr, 
            struct pisces_xpmem_attach * dest_addr,
            struct mm_struct * mm,
            u64 * p_num_pfns, 
            struct xpmem_pfn ** p_pfns
) {

    u64 pfn = 0;
    u64 dest_size = 0;
    u64 num_pfns = 0;
    u64 i = 0;
    u64 vaddr = 0;
    struct xpmem_pfn * pfns = NULL;

    dest_size = dest_addr->size;
    num_pfns = dest_size / PAGE_SIZE;

    pfns = (struct xpmem_pfn *)kmalloc(sizeof(struct xpmem_pfn) * num_pfns, GFP_KERNEL);
    if (!pfns) {
        printk(KERN_ERR "Out of memory\n");
        return -ENOMEM;
    }

    for (i = 0; i < num_pfns; i++) 
    {
        /* Source vaddr must be offset by the destination request */
        vaddr = src_addr->vaddr + dest_addr->offset + (PAGE_SIZE * i);

        pfn = xpmem_vaddr_to_PFN(mm, vaddr);
        if (pfn == 0) {
            printk(KERN_ERR "Could not generated pfn list!\n");
            goto err;
        }

        pfns[i].pfn = pfn;
        printk("vaddr = %p, pfn = %llu (paddr = %p)\n",
            (void *)vaddr, 
            pfn,
            (void *)(pfn << PAGE_SHIFT)
        );
    }

    *p_num_pfns = num_pfns;
    *p_pfns = pfns;
    return 0;

err:
    kfree(pfns);
    *p_num_pfns = 0;
    *p_pfns = NULL;
    return -1;
}
