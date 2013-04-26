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

    char * initrd_path;
    char * kern_path;
    char * kern_cmdline;

    u32 num_cpus;
    u32 boot_cpu;
    cpumask_t assigned_cpus;


    struct pisces_cons cons;

    struct list_head memdesc_list;

};



int pisces_create_enclave(struct pisces_image * img);


#endif
