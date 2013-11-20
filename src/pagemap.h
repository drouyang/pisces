/* Pisces page table management
 * (c) 2013, Brian Kocoloski (briankoco@cs.pitt.edu)
 */

/* We borrow heavily (read: directly) from XPMEM kernel code
 * (http://code.google.com/p/xpmem/)
 */

#ifndef __PAGEMAP_H__
#define __PAGEMAP_H__

#include <linux/types.h>
#include "enclave.h"
#include "pisces_cmds.h"
#include "pisces_portals.h"

int pisces_get_xpmem_map(
        struct pisces_xpmem_make * src_addr, 
        struct pisces_xpmem_attach * dest_addr,
        struct mm_struct * mm,
        u64 * num_pfns, 
        struct xpmem_pfn ** pfns
);


#endif /* __PAGEMAP_H__ */

