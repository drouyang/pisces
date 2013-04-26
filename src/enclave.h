/* Pisces Enclave 
 * (c) 2013, Jack Lange, (jacklange@cs.pitt.edu)
 * (c) 2013, Jiannan Ouyang, (ouyang@cs.pitt.edu)
 */


#ifndef __ENCLAVE_H__
#define __ENCLAVE_H__

#include <linux/types.h>
#include <linux/cpumask.h>

#include "pisces_dev.h"
#include "pisces_cons.h"


struct pisces_enclave {

    // We'll just hold on to the image for now,
    // We need to get rid of this soon though
    struct pisces_image * tmp_image_ptr;

    char * kern_path;
    char * initrd_path;
    char * kern_cmdline;

    u32 num_cpus;
    u32 boot_cpu;
    cpumask_t assigned_cpus;


    struct pisces_cons cons;

    // This is what we will want eventually.....
    struct list_head memdesc_list;

    // But, for now we're just going to hard code it
    uintptr_t base_addr;
    u64 mem_size;
    
};



struct pisces_enclave *  pisces_create_enclave(struct pisces_image * img);


#endif
