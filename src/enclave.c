/* Pisces Enclave 
 * (c) 2013, Jack Lange, (jacklange@cs.pitt.edu)
 * (c) 2013, Jiannan Ouyang, (ouyang@cs.pitt.edu)
 */


#include <linux/slab.h>

#include "enclave.h"
#include "mm.h"

struct pisces_enclave *  pisces_create_enclave(struct pisces_image * img) {

    struct pisces_enclave * enclave = NULL;

    printk(KERN_DEBUG "Creating Pisces Enclave\n");


    enclave = kmalloc(sizeof(struct pisces_enclave), GFP_KERNEL);
    
    if (IS_ERR(enclave)) {
	printk(KERN_ERR "Could not allocate enclave state\n");
	return NULL;
    }


    enclave->tmp_image_ptr = img;

    enclave->kern_path = img->kern_path;
    enclave->initrd_path = img->initrd_path;
    enclave->kern_cmdline = img->cmd_line;

    enclave->mem_size = 128 * 1024 * 1024;
    enclave->base_addr = pisces_alloc_pages((128 * 1024 * 1024) / 4096);

    return enclave;
}
