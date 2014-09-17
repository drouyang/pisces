#include <linux/fs.h>
#include <linux/mutex.h>
#include <linux/percpu.h>
#include <linux/delay.h>

#include <asm/delay.h>
#include <asm/desc.h>
#include <asm/segment.h>
#include <asm/uaccess.h>

#include "pisces_boot_params.h"
#include "enclave.h"
#include "file_io.h"
#include "pisces_ringbuf.h"
#include "enclave_ctrl.h"
#include "pisces_xpmem.h"

#include "boot.h"
#include "pgtables.h"




static inline u32 
sizeof_boot_params(struct pisces_enclave * enclave) 
{
    return sizeof(struct pisces_boot_params);
}



static int 
load_kernel(struct pisces_enclave     * enclave, 
	    struct pisces_boot_params * boot_params, 
	    uintptr_t                   target_addr) 
{
    struct file * kern_image = enclave->kern_file;
    u64           bytes_read = 0;
    
    if (kern_image == NULL) {
	printk(KERN_ERR "Error opening kernel image\n");
	return -1;
    }

    boot_params->kernel_addr = __pa(target_addr);
    boot_params->kernel_size = file_size(kern_image);
    
    while (bytes_read < boot_params->kernel_size) {
	u64 ret = 0;

	ret = file_read(kern_image, 
			(void *)(target_addr + bytes_read), 
			(boot_params->kernel_size - bytes_read), 
			bytes_read);
	
	if (ret <= 0) {
	    printk(KERN_ERR "Error reading kernel image. Only read %llu bytes.\n", bytes_read);
	    file_close(kern_image);
	    return -1;
	}

	bytes_read += ret;
    }

    return 0;
}


static int 
load_initrd(struct pisces_enclave     * enclave, 
	    struct pisces_boot_params * boot_params,
	    uintptr_t                   target_addr) 
{
    struct file * initrd_image = enclave->init_file;
    u64           bytes_read   = 0;
    
    if (initrd_image == NULL) {
	printk(KERN_ERR "Error opening initrd\n");
	return -1;
    }

    boot_params->initrd_addr = __pa(target_addr);
    boot_params->initrd_size = file_size(initrd_image);
    
    while (bytes_read < boot_params->initrd_size) {
	u64 ret = 0;

	ret = file_read(initrd_image, 
			(void *)(target_addr + bytes_read), 
			(boot_params->initrd_size - bytes_read), 
			bytes_read);

	if (ret <= 0) {
	    printk(KERN_ERR "Error reading initrd. Only read %llu bytes.\n", bytes_read);
	    file_close(initrd_image);
	    return -1;
	}

	bytes_read += ret;
    }
    
    printk("Loaded INITRD (bytes_read = %llu)\n", bytes_read);
    printk("INITRD bytes (at %p) = %x\n",         (void *)target_addr, *(unsigned int*)target_addr); 
	   
    return 0;
}


int 
setup_boot_params(struct pisces_enclave * enclave) 
{
    uintptr_t offset = 0;
    uintptr_t base_addr = 0;
    struct pisces_boot_params * boot_params = NULL;

    base_addr   = (uintptr_t)__va(enclave->bootmem_addr_pa);
    boot_params = (struct pisces_boot_params *)base_addr;

    memset((void *)base_addr, 0, enclave->bootmem_size);

    if (!boot_params) {
	printk(KERN_ERR "Invalid address for boot parameters (%p)\n", boot_params);
	return -1;
    }


    printk("Setting up boot parameters. BaseAddr=%p\n", (void *)base_addr);


    /*
     *   copy in loading ASM
     */
    {
        extern u8 launch_code_start[];
	extern u8 launch_code_end[];

	u64 launch_code_size = launch_code_end - launch_code_start;

	if (launch_code_size != sizeof(boot_params->launch_code)) {
	    printk(KERN_ERR "Error: Launch code does not fit boot parameter buffer\n");
	    printk(KERN_ERR "\t launch_code length = %llu, buffer size = %lu\n", 
		   launch_code_size,
		   sizeof(boot_params->launch_code));
	    
	    return -1;
	}

        memcpy(boot_params->launch_code, &launch_code_start, launch_code_size);

        printk("Launch code is at %p\n",
                (void *)__pa(&boot_params->launch_code));
    }


    /*
     *   Basic boot parameters 
     */
    {

	strncpy(boot_params->cmd_line, enclave->kern_cmdline, 1024);

	boot_params->magic              = PISCES_MAGIC;
	boot_params->cpu_id             = enclave->boot_cpu;
	boot_params->apic_id            = apic->cpu_present_to_apicid(enclave->boot_cpu);
	boot_params->cpu_khz            = cpu_khz;   /* Record pre-calculated cpu speed */

	boot_params->trampoline_code_pa = trampoline_state.cpu_init_rip;  /* Linux trampoline address */
	
	boot_params->base_mem_paddr     = enclave->bootmem_addr_pa;
	boot_params->base_mem_size      = enclave->bootmem_size;

	offset += sizeof_boot_params(enclave);

	printk("Linux trampoline at %p\n", (void *)trampoline_state.cpu_init_rip);
	printk("boot params initialized. Offset at %p\n", (void *)(base_addr + offset));
    }


    /*
     *	 Initialize Console Ring buffer (64KB)
     */
    {
	offset = ALIGN(offset, PAGE_SIZE_4KB);
	//    boot_params->console_ring_addr

	if (pisces_cons_init(enclave, (struct pisces_cons_ringbuf *)(base_addr + offset)) == -1) {
	    printk(KERN_ERR "Error initializing Pisces Console\n");
	    return -1;
	}
	
	boot_params->console_ring_addr = __pa(base_addr + offset);
	boot_params->console_ring_size = sizeof(struct pisces_cons_ringbuf);
	
	offset += sizeof(struct pisces_cons_ringbuf);
	printk("console initialized. Offset at %p (target addr=%p, size=%llu)\n", 
	       (void *)(base_addr + offset), 
	       (void *)boot_params->console_ring_addr, boot_params->console_ring_size);
    }


    /*
     * Initialize CMD/CTRL buffer (4KB)
     */
    {
        offset = ALIGN(offset, PAGE_SIZE_4KB);

        boot_params->control_buf_addr = __pa(base_addr + offset);
        boot_params->control_buf_size = PAGE_SIZE_4KB;

        if (pisces_ctrl_init(enclave) == -1) {
            printk(KERN_ERR "Error initializing control channel\n");
            return -1;
        }

        offset += PAGE_SIZE_4KB;

        printk("Control inbuf initialized. Offset at %p (target_addr=%p, size=%llu)\n", 
                (void *)(base_addr + offset),
                (void *)boot_params->control_buf_addr, 
                boot_params->control_buf_size);
    }



    /*
     * Initialize LongCall buffer (4KB)
     */
    {
        offset = ALIGN(offset, PAGE_SIZE_4KB);

        boot_params->longcall_buf_addr = __pa(base_addr + offset);
        boot_params->longcall_buf_size = PAGE_SIZE_4KB;

        if (pisces_lcall_init(enclave) == -1) {
            printk(KERN_ERR "Error initializing Longcall channel\n");
            return -1;
        }

        offset += PAGE_SIZE_4KB;

        printk("Longcall buffer initialized. Offset at %p (target_addr=%p, size=%llu)\n", 
                (void *)(base_addr + offset),
                (void *)boot_params->longcall_buf_addr, 
                boot_params->longcall_buf_size);
    }

    /*
     * Initialize XPMEM buffer (4KB)
     */
    {
        offset = ALIGN(offset, PAGE_SIZE_4KB);

        boot_params->xpmem_buf_addr = __pa(base_addr + offset);
        boot_params->xpmem_buf_size = PAGE_SIZE_4KB;

#ifdef USING_XPMEM
	if (pisces_xpmem_init(enclave) == -1) {
	    printk(KERN_ERR "Error initializing XPMEM channel\n");
	    return -1;
	}
#endif

        offset += PAGE_SIZE_4KB;

        printk("XPMEM buffer initialized. Offset at %p (target_addr=%p, size=%llu)\n", 
                (void *)(base_addr + offset),
                (void *)boot_params->xpmem_buf_addr, 
                boot_params->xpmem_buf_size);
    }


    /*
     * 1. kernel image
     */
    {
	offset = ALIGN(offset, PAGE_SIZE_2MB);

	printk("Loading Kernel. Offset at %p\n", (void *)(base_addr + offset));

	if (offset != PAGE_SIZE_2MB) {
	    printk(KERN_ERR "Error: Kitten kernel must be loaded at the 2MB offset\n");
	    printk(KERN_ERR "\t This can only be changed if you update CONFIG_PHYSICAL_START\n" \
		            "\t in the Kitten configuration, and update the offset check in pisces_boot_params.c\n");
	    return -1;
	}
	
	if (load_kernel(enclave, boot_params, base_addr + offset) == -1) {
	    printk(KERN_ERR "Error loading kernel to target (%p)\n", (void *)(base_addr + offset));
	    return -1;
	}
	
	offset += boot_params->kernel_size;

	printk("\t kernel loaded. Offset at %p\n", (void *)(base_addr + offset));
    }



    /*
     * Lets just pad some space here for the kernel's BSS...
     *    JRL TODO: How do we deal with this permanently???
     */
    offset += 4 * PAGE_SIZE_2MB;



    /* 
     * InitRD
     */
    {
	offset = ALIGN(offset, PAGE_SIZE_2MB);
	
	printk("Loading InitRD. Offset at %p\n", (void *)(base_addr + offset));
	if (load_initrd(enclave, boot_params, base_addr + offset) == -1) {
	    printk(KERN_ERR "Error loading initrd to target (%p)\n", (void *)(base_addr + offset));
	    return -1;
	}
	
	offset += boot_params->initrd_size;

	printk("\tInitRD loaded. Offset at %p\n", (void *)(base_addr + offset));
    }


    printk(KERN_INFO "Pisces loader memroy map:\n");
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


