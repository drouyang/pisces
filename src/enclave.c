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
#include "pisces.h"
#include "file_io.h"


static inline u32 sizeof_boot_params(struct pisces_enclave * enclave) {
    return ( sizeof(struct pisces_boot_params) + 
	     (sizeof(struct pisces_mmap_entry) * enclave->memdesc_num));
}



// setup bootstrap page tables - pisces_ident_pgt
// 4M identity mapping from mem_base
static int setup_ident_pts(struct pisces_enclave * enclave,
			   struct pisces_boot_params * boot_params, 
			   uintptr_t target_addr) {
    u64 addr = 0;
    int i = 0;
    struct pisces_ident_pgt * ident_pgt = (struct pisces_ident_pgt *)target_addr;
    memset(ident_pgt, 0, sizeof(struct pisces_ident_pgt));

    boot_params->level3_ident_pgt = __pa(target_addr);

    addr = PAGE_TO_BASE_ADDR(__pa(ident_pgt->pd));
    ident_pgt->pdp[PDPE64_INDEX(__va(enclave->base_addr_pa))].pd_base_addr = addr;
    ident_pgt->pdp[PDPE64_INDEX(__va(enclave->base_addr_pa))].present   = 1;
    ident_pgt->pdp[PDPE64_INDEX(__va(enclave->base_addr_pa))].writable  = 1;
    ident_pgt->pdp[PDPE64_INDEX(__va(enclave->base_addr_pa))].accessed  = 1;

    // 512 * 2M = 1G
    for (i = 0, addr = enclave->base_addr_pa; 
            i < MAX_PDE64_ENTRIES; 
            i++, addr += PAGE_SIZE_2MB) {
        addr = PAGE_TO_BASE_ADDR_2MB(addr);
        ident_pgt->pd[PDE64_INDEX(addr)].page_base_addr  = addr;
        ident_pgt->pd[PDE64_INDEX(addr)].present         = 1;
        ident_pgt->pd[PDE64_INDEX(addr)].writable        = 1;
        ident_pgt->pd[PDE64_INDEX(addr)].accessed        = 1;
        ident_pgt->pd[PDE64_INDEX(addr)].large_page      = 1;
    }

    return 0;
}



static int load_kernel(struct pisces_enclave * enclave, 
		       struct pisces_boot_params * boot_params, 
		       uintptr_t target_addr) {

    struct file * kern_image = file_open(enclave->kern_path, O_RDONLY);
    u64 bytes_read = 0;
    
    if (kern_image == NULL) {
	printk(KERN_ERR "Error opening kernel image (%s)\n", enclave->kern_path);
	return -1;
    }

    boot_params->kernel_addr = target_addr;
    boot_params->kernel_size = file_size(kern_image);
    
    while (bytes_read < boot_params->kernel_size) {
	u64 ret = 0;

	ret = file_read(kern_image, (void *)(target_addr + bytes_read), 
			(boot_params->kernel_size - bytes_read), 
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
    
    if (initrd_image == NULL) {
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

    printk("enclave=%p, boot_params=%p\n", enclave, boot_params);

    printk(KERN_INFO "PISCES: setting up memory map:\n");

    for (i = 0; i < enclave->memdesc_num; i++) {
	boot_params->mmap[i].addr = enclave->base_addr_pa;
	boot_params->mmap[i].size = enclave->mem_size;

        printk(KERN_INFO "  %d: [0x%llx, 0x%llx),  size 0x%llx\n",
	       i,
	       boot_params->mmap[i].addr,
	       boot_params->mmap[i].addr + boot_params->mmap[i].size,
	       boot_params->mmap[i].size);
    }

    return 0;
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


    // copy in loading ASM
    {
	extern u8 launch_code_start;
	extern u8 launch_code_end;
	printk("Launch code is at %p, end is at %p\n", &launch_code_start, &launch_code_end);

	printk("Launch code is %lu bytes\n", ((uintptr_t)&launch_code_end - (uintptr_t)&launch_code_start));
	memcpy(boot_params->launch_code, &launch_code_start, (u64)&launch_code_end - (u64)&launch_code_start);
    }    


    boot_params->magic = PISCES_MAGIC;
    strncpy(boot_params->cmd_line, enclave->kern_cmdline, 1024);


    // Initialize Memory map
    // Hardcode the mmap size to 1 for now
    if (setup_mmap(enclave, boot_params) == -1) {
	printk(KERN_ERR "Error setting up memory map\n");
	return -1;
    }

    offset += sizeof_boot_params(enclave);
    printk("boot params initialized. Offset at %p\n", (void *)(base_addr + offset));
    

    // Initialize Console Ring buffer (64KB)
    offset += ALIGN(offset, PAGE_SIZE_4KB);
    //    boot_params->console_ring_addr
    if (pisces_cons_init(enclave, (struct pisces_cons_ringbuf *)(base_addr + offset)) == -1) {
	printk(KERN_ERR "Error initializing Pisces Console\n");
	return -1;
    }
    
    boot_params->console_ring_addr = __pa(base_addr + offset);
    boot_params->console_ring_size = sizeof(struct pisces_cons_ringbuf);

    offset += sizeof(struct pisces_cons_ringbuf);
    printk("console initialized. Offset at %p\n", (void *)(base_addr + offset));
    

    // Identity mapped page tables
    offset = ALIGN(offset, PAGE_SIZE_4KB);
    printk("Setting up Ident PTS at Offset at %p\n", (void *)(base_addr + offset));

    if (setup_ident_pts(enclave, boot_params, base_addr + offset) == -1) {
	printk(KERN_ERR "Error configuring identity mapped page tables\n");
	return -1;
    }
    offset += sizeof(struct pisces_ident_pgt);

    printk("\t Ident PTS done.Offset at %p\n", (void *)(base_addr + offset));


    // 1. kernel image
    offset = ALIGN(offset, PAGE_SIZE_2MB);

    printk("Loading Kernel. Offset at %p\n", (void *)(base_addr + offset));
    if (load_kernel(enclave, boot_params, base_addr + offset) == -1) {
	printk(KERN_ERR "Error loading kernel to target (%p)\n", (void *)(base_addr + offset));
	return -1;
    }

    offset += boot_params->kernel_size;
    printk("\t kernel loaded. Offset at %p\n", (void *)(base_addr + offset));
    

    // 2. initrd 
    offset = ALIGN(offset, PAGE_SIZE_2MB);

    printk("Loading InitRD. Offset at %p\n", (void *)(base_addr + offset));
    if (load_initrd(enclave, boot_params, base_addr + offset) == -1) {
	printk(KERN_ERR "Error loading initrd to target (%p)\n", (void *)(base_addr + offset));
	return -1;
    }

    offset += boot_params->initrd_size;

    printk("\tInitRD loaded. Offset at %p\n", (void *)(base_addr + offset));

    printk(KERN_INFO "PISCES: loader memroy map:\n");
    printk(KERN_INFO "  kernel:        [%p, %p), size %llu\n",
	   (void *)boot_params->kernel_addr,
	   (void *)(boot_params->kernel_addr + boot_params->kernel_size),
            boot_params->kernel_size);
    printk(KERN_INFO "  initrd:        [%p, %p), size %llu\n",
	   (void *)boot_params->initrd_addr, 
	   (void *)(boot_params->initrd_addr + boot_params->initrd_size),
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

    enclave->memdesc_num = 1;
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
