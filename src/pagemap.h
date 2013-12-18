/* Pisces page table management
 * (c) 2013, Brian Kocoloski (briankoco@cs.pitt.edu)
 */

/* We borrow heavily (read: directly) from XPMEM kernel code
 * (http://code.google.com/p/xpmem/)
 */

#ifndef __PAGEMAP_H__
#define __PAGEMAP_H__

#include <linux/types.h>

struct enclave_aspace {
    u64 cr3;
};

struct xpmem_pfn {
    u64 pfn;
};

struct pisces_xpmem_make;
struct pisces_xpmem_attach;

int pisces_get_xpmem_map(
        struct pisces_xpmem_make * src_addr, 
        struct pisces_xpmem_attach * dest_addr,
        struct mm_struct * mm,
        u64 * num_pfns, 
        struct xpmem_pfn ** pfns
);

int pisces_get_enclave_xpmem_map(
        struct pisces_xpmem_make * src_addr, 
        struct pisces_xpmem_attach * dest_addr,
        struct enclave_aspace * aspace,
        u64 * num_pfns, 
        struct xpmem_pfn ** pfns
);

unsigned long pisces_map_xpmem_pfn_range(
    struct xpmem_pfn * pfns,
    u64 num_pfns
);


#endif /* __PAGEMAP_H__ */

