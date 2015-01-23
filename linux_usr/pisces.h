/* Pisces Interface Library 
 * (c) 2014, Jack Lange <jacklange@cs.pitt.edu>
 */

#ifndef __PISCES_H__
#define __PISCES_H__

#include <pet_types.h>


#define PISCES_ANY_CPU       -1
#define PISCES_ANY_MEMBLOCK  -1
#define PISCES_ANY_NUMA_ZONE -1


int pisces_load(char * kern, char * initrd, char * cmd_line);

int pisces_launch(int pisces_id, 
		  int cpu_id, 
		  int block_id, 
		  int numa_zone, 
		  int num_blocks);


int 
pisces_get_cons_fd(int pisces_id);

int 
pisces_reset(int pisces_id);


static inline char * 
get_pisces_dev_path(int pisces_id) 
{
    char * dev_path = NULL;
    
    asprintf(&dev_path, "/dev/pisces-enclave%d", pisces_id);

    return dev_path;
}

static inline int
get_pisces_id_from_path(char * dev_path)
{
    int pisces_id = -1;

    if (sscanf(dev_path, "/dev/pisces-enclave%d", &pisces_id) != 1) {
	return -1;
    }

    return pisces_id;
}


#endif
