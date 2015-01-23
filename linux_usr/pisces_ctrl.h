/* Pisces Interface Library 
 * (c) 2014, Jack Lange <jacklange@cs.pitt.edu>
 */

#ifndef __PISCES_CTRL_H__
#define __PISCES_CTRL_H__

#include <pet_types.h>

int 
pisces_add_mem_node(int pisces_id, 
		    int numa_zone);


int 
pisces_add_mem(int pisces_id, 
	       int num_blocks, 
	       int numa_zone);

int 
pisces_add_mem_explicit(int pisces_id,
			int block_id);

int 
pisces_add_cpus(int pisces_id,
		int num_cpus, 
		int numa_zone);

int 
pisces_add_cpu(int      pisces_id,
	       uint64_t cpu_id);

int 
pisces_remove_cpu(int      pisces_id,
		  uint64_t cpu_id);


int
pisces_run_job(int         pisces_id,
	       char      * name, 
	       char      * exe_path,
	       char      * exe_argv,
	       char      * envp,
	       uint8_t     use_large_pages,
	       uint8_t     use_smartmap,
	       uint8_t     num_ranks,
	       uint64_t    cpu_mask,
	       uint64_t    heap_size,
	       uint64_t    stack_size);

#endif
