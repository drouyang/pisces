/* 
 * V3/Pisces VM console
 * (c) 2013, Jack Lange, <jacklange@cs.pitt.edu>
 */


#ifndef __V3_CONSOLE__
#define __V3_CONSOLE__

#include <linux/types.h>


struct pisces_enclave;

int v3_console_connect(struct pisces_enclave * enclave, 
		       u32 vm_id, uintptr_t cons_buf_pa);




#endif
