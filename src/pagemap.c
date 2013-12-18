/* Pisces page table management
 * (c) 2013, Brian Kocoloski (briankoco@cs.pitt.edu)
 */

/* We borrow heavily (read: directly) from XPMEM kernel code
 * (http://code.google.com/p/xpmem/)
 */

#include <linux/hugetlb.h>
#include <linux/mm.h>
#include <linux/mman.h>
#include "pagemap.h"
#include "pisces_portals.h"

#define enclave_pgd_offset(aspace, addr) &((pgd_t *)__va(aspace->cr3))[pgd_index(addr)]

#if 0
pte_t *
xpmem_huge_pte_offset(
    struct mm_struct *mm, 
    struct enclave_aspace * aspace, 
    unsigned long addr
)
{
    pgd_t *pgd;
    pud_t *pud;
    pmd_t *pmd = NULL;

    if (mm) 
        pgd = pgd_offset(mm, addr);
    else if (aspace)
        pgd = enclave_pgd_offset(aspace, addr);
    else
        return NULL;

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


pte_t *
xpmem_hugetlb_pte(
    struct mm_struct * mm, 
    struct enclave_aspace * aspace, 
    u64 vaddr, 
    u64 * offset
)
{
    struct vm_area_struct * vma;
    u64 address;
    pte_t * pte;

    /* No hugetlb pages in the enclave */
    if (aspace)
        return NULL;

    vma = find_vma(mm, vaddr);
    if (!vma)
        return NULL;

    if (is_vm_hugetlb_page(vma)) {
        struct hstate * hs = hstate_vma(vma);

        address = vaddr & huge_page_mask(hs);
        if (offset) {
            *offset = (vaddr & (huge_page_size(hs) - 1)) & PAGE_MASK;
        }

        pte = xpmem_huge_pte_offset(mm, aspace, address);
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
#endif

/*
 * Given an address space and a virtual address return a pointer to its 
 * pte if one is present.
 */
static inline pte_t *
xpmem_vaddr_to_pte_offset(
    struct mm_struct * mm, 
    struct enclave_aspace * aspace,
    u64 vaddr, 
    u64 * offset
) 
{
    pgd_t * pgd;
    pud_t * pud;
    pmd_t * pmd;
    pte_t * pte;

    if (offset)
        *offset = 0;

    if (mm)
        pgd = pgd_offset(mm, vaddr);
    else if (aspace)
        pgd = enclave_pgd_offset(aspace, vaddr);
    else
        return NULL;

    printk("gpgd = %p , pgd = %p, pgd_index(%p) = %llu\n",
        (mm != NULL ? (void *)mm->pgd : (void *)aspace->cr3),
        (void *)pgd,
        (void *)vaddr,
        (unsigned long long)pgd_index(vaddr));

    if (!pgd_present(*pgd)) {
        printk("...not present\n");
        return NULL;
    }
    else if (pgd_large(*pgd)) {
        //printk("pgd = %p\n", pgd);
        //return xpmem_hugetlb_pte(mm, aspace, vaddr, offset);
        printk(KERN_ERR "Found large pgd = %p - we do not handle these\n", pgd);
        return NULL;
    }

    pud = pud_offset(pgd, vaddr);
    if (!pud_present(*pud))
        return NULL;
    else if (pud_large(*pud)) {
        //printk("pud = %p\n", pud);
        //return xpmem_hugetlb_pte(mm, aspace, vaddr, offset);
        printk(KERN_ERR "Found large pud = %p - we do not handle these\n", pud);
        return NULL; 
    }    

    pmd = pmd_offset(pud, vaddr);
    if (!pmd_present(*pmd))
        return NULL;
    else if (pmd_large(*pmd)) {
        //printk("pmd = %p\n", pmd);
        //return xpmem_hugetlb_pte(mm, aspace, vaddr, offset);
        printk(KERN_ERR "Found large pmd = %p - we do not handle these\n", pmd);
        return NULL; 
    }    

    pte = pte_offset_map(pmd, vaddr);
    if (!pte_present(*pte))
        return NULL;

    return pte;
}

static inline u64
xpmem_vaddr_to_PFN(
    struct mm_struct * mm, 
    struct enclave_aspace * aspace,
    u64 vaddr
)
{
    pte_t * pte;
    u64 pfn, offset;

    pte = xpmem_vaddr_to_pte_offset(mm, aspace, vaddr, &offset);
    if (pte == NULL)
        return 0;

    pfn = pte_pfn(*pte) + (offset >> PAGE_SHIFT);

    return pfn;
}


static int get_xpmem_map(
    struct pisces_xpmem_make * src_addr,
    struct pisces_xpmem_attach * dest_addr,
    struct mm_struct * mm,
    struct enclave_aspace * aspace,
    u64 * p_num_pfns,
    struct xpmem_pfn ** p_pfns
) 
{
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

        pfn = xpmem_vaddr_to_PFN(mm, aspace, vaddr);
        if (pfn == 0) {
            printk(KERN_ERR "Could not generate pfn list!\n");
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


/* Set the page map for the PFN range in xpmem_addr's source address space */
int pisces_get_xpmem_map(
    struct pisces_xpmem_make * src_addr, 
    struct pisces_xpmem_attach * dest_addr,
    struct mm_struct * mm,
    u64 * p_num_pfns, 
    struct xpmem_pfn ** p_pfns
)
{
    return get_xpmem_map(src_addr, dest_addr, mm, NULL, p_num_pfns, p_pfns);
}

int pisces_get_enclave_xpmem_map(
    struct pisces_xpmem_make * src_addr, 
    struct pisces_xpmem_attach * dest_addr,
    struct enclave_aspace * aspace,
    u64 * p_num_pfns, 
    struct xpmem_pfn ** p_pfns
) {
    return get_xpmem_map(src_addr, dest_addr, NULL, aspace, p_num_pfns, p_pfns);
}


unsigned long
pisces_map_xpmem_pfn_range(
    struct xpmem_pfn * pfns,
    u64 num_pfns
)
{
    struct vm_area_struct * vma = NULL;
    unsigned long addr = 0;
    unsigned long attach_addr = 0;
    unsigned long size = 0;
    u64 i = 0;
    int status = 0;

    size = num_pfns * PAGE_SIZE;
    attach_addr = vm_mmap(NULL, 0, size, PROT_READ | PROT_WRITE,
            MAP_SHARED, 0);
    if (IS_ERR_VALUE(attach_addr)) {
        printk(KERN_ERR "vm_mmap failed!\n");
        return attach_addr;
    }

    vma = find_vma(current->mm, attach_addr);
    if (!vma) {
        printk(KERN_ERR "find_vma failed - this should be impossible\n");
        return -ENOMEM;
    }

    for (i = 0; i < num_pfns; i++) {
        addr = attach_addr + (i * PAGE_SIZE);

        printk("Mapping vaddr = %p, pfn = %llu (paddr = %p)\n",
            (void *)addr, 
            pfns[i].pfn,
            (void *)(pfns[i].pfn << PAGE_SHIFT)
        );
        status = remap_pfn_range(vma, addr, pfns[i].pfn, PAGE_SIZE, vma->vm_page_prot);
        if (status != 0) {
            return -ENOMEM;
        }
    }

    return attach_addr;
}
