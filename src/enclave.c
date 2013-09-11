/* Pisces Enclave 
 * (c) 2013, Jack Lange, (jacklange@cs.pitt.edu)
 * (c) 2013, Jiannan Ouyang, (ouyang@cs.pitt.edu)
 */


#include <linux/slab.h>
#include <linux/kernel.h>

#include "enclave.h"
#include "mm.h"
#include "boot.h"


#include "pgtables.h"





struct pisces_enclave *  pisces_create_enclave(struct pisces_image * img) {

    struct pisces_enclave * enclave = NULL;

    printk(KERN_DEBUG "Creating Pisces Enclave\n");

    enclave = kmalloc(sizeof(struct pisces_enclave), GFP_KERNEL);
    
    if (IS_ERR(enclave)) {
	printk(KERN_ERR "Could not allocate enclave state\n");
	return NULL;
    }

    memset(enclave, 0, sizeof(struct pisces_enclave));

    enclave->tmp_image_ptr = img;

    enclave->kern_path = img->kern_path;
    enclave->initrd_path = img->initrd_path;
    enclave->kern_cmdline = img->cmd_line;

    enclave->bootmem_size = 128 * 1024 * 1024;
    enclave->bootmem_addr_pa = pisces_alloc_pages((128 * 1024 * 1024) / PAGE_SIZE_4KB);
    memset(__va(enclave->bootmem_addr_pa), 0, 128 * 1024 * 1024);



    /* Setup control device in /dev */

    return enclave;
}


int pisces_launch_enclave(struct pisces_enclave * enclave) {

    enclave->boot_cpu = 1;

    if (setup_boot_params(enclave) == -1) {
	printk(KERN_ERR "Error setting up boot environment\n");

	return -1;
    }


    if (boot_enclave(enclave) == -1) {
	printk(KERN_ERR "Error booting enclave\n");
	return -1;
    }

    return 0;

}



void pisces_free_enclave(struct pisces_enclave * enclave) {

    pisces_free_pages(enclave->bootmem_addr_pa, (128 * 1024 * 1024) / PAGE_SIZE_4KB);
    kfree(enclave);

    return;
}
