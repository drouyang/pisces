
/* Pisces Memory Management
 * (c) 2013, Jack Lange, (jacklange@cs.pitt.edu)
 * (c) 2013, Jiannan Ouyang, (ouyang@cs.pitt.edu)
 */


#ifndef __MM_H__
#define __MM_H__


struct memdesc {
    
    uintptr_t base_addr;
    u32 order;

    struct list_head node;
};




#endif
