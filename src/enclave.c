/* Pisces Enclave 
 * (c) 2013, Jack Lange, (jacklange@cs.pitt.edu)
 * (c) 2013, Jiannan Ouyang, (ouyang@cs.pitt.edu)
 */


#include <linux/slab.h>
#include <linux/kernel.h>

#include "enclave.h"
#include "mm.h"
#include "pgtables.h"
#include "pisces_cons.h"



static inline u32 sizeof_boot_params(struct enclave * enclave) {
    return ( sizeof(struct pisces_boot_params) + 
	     (sizeof(struct pisces_mmap_entry) * enclave->memdesc_num));
}


static int load_kernel(struct pisces_enclave * enclave, 
		       struct pisces_boot_params * boot_params, 
		       uintptr_t target_addr) {

    struct file * kern_image = file_open(enclave->kern_path, O_RDONLY);
    u64 bytes_read = 0;
    
    if (IS_ERR(kern_image)) {
	printk(KERN_ERR "Error opening kernel image (%s)\n", enclave->kern_path);
	return -1;
    }

    boot_params->kernel_addr = target_addr;
    boot_params->kernel_size = file_size(kern_image);
    
    while (bytes_read < boot_params->kern_size) {
	u64 ret = 0;

	ret = file_read(kern_image, (void *)(target_addr + bytes_read), 
			(boot_params->kern_size - bytes_read), 
			bytes_read);

	if (ret == 0) {
	    printk(KERN_ERR "Error reading kernel image. Only read %llu bytes.\n", bytes_read);
	    file_close(kern_image);
	    return -1;
	}

	bytes_read += ret;
    }

    file_close(kern_image);

    return 0;
}


static int load_initrd(struct pisces_enclave * enclave, 
		       struct pisces_boot_params * boot_params,
		       uintptr_t target_addr) {


    struct file * initrd_image = file_open(enclave->initrd_path, O_RDONLY);
    u64 bytes_read = 0;
    
    if (IS_ERR(initrd_image)) {
	printk(KERN_ERR "Error opening initrd (%s)\n", enclave->initrd_path);
	return -1;
    }

    boot_params->initrd_addr = target_addr;
    boot_params->initrd_size = file_size(initrd_image);
    
    while (bytes_read < boot_params->initrd_size) {
	u64 ret = 0;

	ret = file_read(initrd_image, (void *)(target_addr + bytes_read), 
			(boot_params->initrd_size - bytes_read), 
			bytes_read);

	if (ret == 0) {
	    printk(KERN_ERR "Error reading initrd. Only read %llu bytes.\n", bytes_read);
	    file_close(initrd_image);
	    return -1;
	}

	bytes_read += ret;
    }

    file_close(initrd_image);

    return 0;
}



static int setup_mmap(struct pisces_enclave * enclave, struct pisces_boot_params * boot_params) {
    int i;

    boot_params->num_mmap_entries = enclave->memdesc_num;

    printk(KERN_INFO "PISCES: setting up memory map:\n");

    for (i = 0; i < enclave->memdesc_num; i++) {
	boot_params->mmap[i].addr = enclave->base_addr;
	boot_params->mmap[i].size = enclave->mem_size;

        printk(KERN_INFO "  %d: [0x%llx, 0x%llx),  size 0x%llx\n",
	       i,
	       boot_params->mmap[i].addr,
	       boot_params->mmap[i].addr + boot_params->mmap[i].size,
	       boot_params->mmap[i].size);
    }

    return 0
}

static int setup_boot_params(struct pisces_enclave * enclave) {
    uintptr_t offset = 0;
    uintptr_t base_addr = 0;
    struct pisces_boot_params * boot_params = NULL;

    base_addr = (uintptr_t)__va(enclave->base_addr_pa);
    boot_params = (struct pisces_boot_params *)base_addr;

    if (!boot_params) {
	printk(KERN_ERR "Invalid address for boot parameters (%p)\n", boot_params);
	return -1;
    }


    boot_params->magic = PISCES_MAGIC;
    strncpy(boot_params->cmd_line, enclave->cmd_line, 1024);


    // Initialize Memory map
    // Hardcode the mmap size to 1 for now
    if (setup_mmap(enclave, boot_params) == -1) {
	printk(KERN_ERR "Error setting up memory map\n");
	return -1;
    }

    offset += sizeof_boot_params(enclave);

    // Initialize Console Ring buffer (64KB)
    offset = ALIGN(offset, PAGE_SIZE_4KB);
    boot_params->console_ring_addr

    offset += sizeof(struct pisces_cons_ringbuf);
    

    // 1. kernel image
    offset = ALIGN(offset, PAGE_SIZE_2MB);

    if (load_kernel(enclave, boot_params, base_addr + offset) == -1) {
	printk(KERN_ERR "Error loading kernel to target (%p)\n", (void *)(base_addr + offset));
	return -1;
    }

    offset += boot_params->kernel_size;
    

    // 2. initrd 
    offset = ALIGN(offset, PAGE_SIZE_2MB);

    if (load_initrd(enclave, boot_params, base_addr + offset) == -1) {
	printk(KERN_ERR "Error loading initrd to target (%p)\n", (void *)(base_addr + offset));
	return -1;
    }

    offset += boot_params->initrd_size;


    printk(KERN_INFO "PISCES: loader memroy map:\n");
    printk(KERN_INFO "  kernel:        [0x%lx, 0x%lx), size 0x%lx\n",
            boot_params->kernel_addr,
            boot_params->kernel_addr + boot_params->kernel_size,
            boot_params->kernel_size);
    printk(KERN_INFO "  initrd:        [0x%lx, 0x%lx), size 0x%lx\n",
            boot_params->initrd_addr, 
            boot_params->initrd_addr + boot_params->initrd_size,
            boot_params->initrd_size);

    
    return 0;
}



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

    enclave->mem_size = 128 * 1024 * 1024;
    enclave->base_addr_pa = pisces_alloc_pages((128 * 1024 * 1024) / PAGE_SIZE_4KB);

    memset(__va(enclave->base_addr_pa), 0, 128 * 1024 * 1024);


    // memory init goes first because we need to setup share_info
    // region first
    if (setup_boot_params(enclave) == -1) {
	printk(KERN_ERR "Error setting up boot environment\n");

	kfree(enclave);
	return NULL;
    }

    return enclave;
}



void pisces_free_enclave(struct pisces_enclave * enclave) {

    pisces_free_pages(enclave->base_addr_pa, (128 * 1024 * 1024) / PAGE_SIZE_4KB);
    kfree(enclave);

    return;
}
