
/* Pisces Memory Management
 * (c) 2013, Jack Lange, (jacklange@cs.pitt.edu)
 * (c) 2013, Jiannan Ouyang, (ouyang@cs.pitt.edu)
 */



#ifndef __MM_H__
#define __MM_H__

#include <linux/types.h>


struct memdesc {
    
    uintptr_t base_addr;
    u32 order;

    struct list_head node;
};

int pisces_mem_init( void );


uintptr_t pisces_alloc_pages_on_node(u64 num_pages, int node_id);
uintptr_t pisces_alloc_pages(u64 num_pages);
void pisces_free_pages(uintptr_t page_addr, u64 num_pages);


int pisces_add_mem(uintptr_t base_addr, u64 num_pages);


#endif
