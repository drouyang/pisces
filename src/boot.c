#include <linux/fs.h>
#include <linux/mutex.h>
#include <linux/percpu.h>
#include <linux/kallsyms.h>
#include <asm/desc.h>
#include <asm/segment.h>
#include <asm/uaccess.h>
#include <asm/realmode.h>

#include "boot_params.h"
#include "enclave.h"
#include "file_io.h"


extern int wakeup_secondary_cpu_via_init(int, unsigned long);



static inline u32 sizeof_boot_params(struct pisces_enclave * enclave) {
    return sizeof(struct pisces_boot_params);
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
    ident_pgt->pdp[PDPE64_INDEX(__va(enclave->bootmem_addr_pa))].pd_base_addr = addr;
    ident_pgt->pdp[PDPE64_INDEX(__va(enclave->bootmem_addr_pa))].present   = 1;
    ident_pgt->pdp[PDPE64_INDEX(__va(enclave->bootmem_addr_pa))].writable  = 1;
    ident_pgt->pdp[PDPE64_INDEX(__va(enclave->bootmem_addr_pa))].accessed  = 1;

    // 512 * 2M = 1G
    for (i = 0, addr = enclave->bootmem_addr_pa; 
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

    boot_params->kernel_addr = __pa(target_addr);
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

    boot_params->initrd_addr = __pa(target_addr);
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
    
    printk("Loaded INITRD %s (bytes_read = %llu)\n", enclave->initrd_path, bytes_read);
    printk("INITRD bytes (at %p) = %x\n", (void *)target_addr, *(unsigned int*)target_addr); 
	   
	



    file_close(initrd_image);

    return 0;
}




int setup_boot_params(struct pisces_enclave * enclave) {
    uintptr_t offset = 0;
    uintptr_t base_addr = 0;
    struct pisces_boot_params * boot_params = NULL;

    base_addr = (uintptr_t)__va(enclave->bootmem_addr_pa);
    boot_params = (struct pisces_boot_params *)base_addr;

    if (!boot_params) {
	printk(KERN_ERR "Invalid address for boot parameters (%p)\n", boot_params);
	return -1;
    }


    printk("Setting up boot parameters. BaseAddr=%p\n", (void *)base_addr);


    /*
     *   copy in loading ASM
     */
    {
	extern u8 launch_code_start;
	extern u8 launch_code_end;


	memcpy(boot_params->launch_code, &launch_code_start, (u64)&launch_code_end - (u64)&launch_code_start);

	printk("Launch code is at 0x%p, size %lu bytes\n",
                (void *)__pa(&boot_params->launch_code), 
                ((u8 *)&launch_code_end - (u8 *)&launch_code_start));

    }    


    /*
     *   Basic boot parameters 
     */
    {
	boot_params->magic = PISCES_MAGIC;
	strncpy(boot_params->cmd_line, enclave->kern_cmdline, 1024);
	
	
	boot_params->cpu_id = enclave->boot_cpu;
	// Record pre-calculated cpu speed
	boot_params->cpu_khz = cpu_khz;

	boot_params->base_mem_paddr = enclave->bootmem_addr_pa;
	boot_params->base_mem_size = enclave->bootmem_size;

	offset += sizeof_boot_params(enclave);
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
     * Initialize CMD/CTRL ring buffer (4KB)
     */
    /*
    {
	offset = ALIGN(offset, PAGE_SIZE_4KB);

	if (pisces_ctrl_init(enclave, (struct pisces_ctrl_ringbuf *)(base_addr + offset)) == -1) {
	    printk(KERN_ERR "Error initializing control channel\n");
	    return -1;
	}

	boot_params->control_ring_addr = __pa(base_addr + offset);
	boot_params->control_ring_size = sizeof(struct pisces_ctrl_ringbuf);

	offset += sizeof(pisces_ctrl_ringbuf);
	printk("Control channel initialized. Offset at %p (target_addr=%p, size=%llu)\n", 
	       (void *)(base_addr + offset),
	       (void *)boot_params->control_ring_addr, boot_params->control_ring_size);
	
    }
    */

    /* 
     * 	Identity mapped page tables
     */
    {

	offset = ALIGN(offset, PAGE_SIZE_4KB);
	printk("Setting up Ident PTS at Offset at %p\n", (void *)(base_addr + offset));
	
	if (setup_ident_pts(enclave, boot_params, base_addr + offset) == -1) {
	    printk(KERN_ERR "Error configuring identity mapped page tables\n");
	    return -1;
	}
	offset += sizeof(struct pisces_ident_pgt);
	
	printk("\t Ident PTS done.Offset at %p\n", (void *)(base_addr + offset));
    }


    /*
     * 1. kernel image
     */
    {
	offset = ALIGN(offset, PAGE_SIZE_2MB);
	
	printk("Loading Kernel. Offset at %p\n", (void *)(base_addr + offset));
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
    offset += PAGE_SIZE_2MB;



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

/*
 * TODO: cpu_add(enclave)
 * 1. set lanuch code
 * 2. reset cpu
 */

int boot_enclave(struct pisces_enclave * enclave) {
    int ret = 0;
    int apicid = apic->cpu_present_to_apicid(enclave->boot_cpu);
    struct pisces_boot_params * boot_params = (struct pisces_boot_params *)__va(enclave->bootmem_addr_pa);

    printk(KERN_DEBUG "Boot Pisces guest cpu\n");

    // patch launch_code
    // TODO: independent function
    {
	extern u8 launch_code_start;
	extern u8 launch_code_end;

        extern u64 launch_code_target_addr;
        extern u64 launch_code_esi;
        u64 * target_addr_ptr = NULL;
        u64 * esi_ptr = NULL;

	printk("Patching Launch Code (length = %d bytes)\n", 
	       (u32)((u8 *)&launch_code_end - (u8 *)&launch_code_start));
	
        // setup launch code data
        target_addr_ptr =  (u64 *)((u8 *)&boot_params->launch_code 
            + ((u8 *)&launch_code_target_addr - (u8 *)&launch_code_start));
        
	esi_ptr =  (u64 *)((u8 *)&boot_params->launch_code 
            + ((u8 *)&launch_code_esi - (u8 *)&launch_code_start));
        
	*target_addr_ptr = boot_params->kernel_addr;
        *esi_ptr = (enclave->bootmem_addr_pa >> PAGE_SHIFT);
       
	printk(KERN_DEBUG "  patch target address 0x%p at 0x%p\n", 
                (void *) *target_addr_ptr, (void *) __pa(target_addr_ptr));
        
	printk(KERN_DEBUG "  patch esi 0x%p at 0x%p\n", 
                (void *) *esi_ptr, (void *) __pa(esi_ptr));
    }


    // setup linux trampoline
    // TODO: independent function
    {
        u64 header_addr = kallsyms_lookup_name("real_mode_header");
        struct real_mode_header * real_mode_header = *(struct real_mode_header **)header_addr;
        struct trampoline_header * trampoline_header = (struct trampoline_header *) __va(real_mode_header->trampoline_header);

        pml4e64_t * pml = (pml4e64_t *) __va(real_mode_header->trampoline_pgd);
        pml4e64_t tmp_pml0;

        u64 start_ip = real_mode_header->trampoline_start;

        u64 cpu_maps_update_lock_addr =  kallsyms_lookup_name("cpu_add_remove_lock");
        struct mutex * cpu_maps_update_lock = (struct mutex *)cpu_maps_update_lock_addr;

        printk(KERN_DEBUG "real_mode_header addr 0x%p", (void *)header_addr);
        printk(KERN_DEBUG "cpu_add_remove_lock addr 0x%p", (void *)cpu_maps_update_lock_addr);
        /* 
         * setup page table used by linux trampoline
         */
        // hold this lock to serialize trampoline data access
        // this is the same as cpu_maps_update_begin/done API
        mutex_lock(cpu_maps_update_lock);

        // backup old pml[0] which points to level3_ident_pgt that maps 1G
        tmp_pml0 = pml[0];

        // use the level3_ident_pgt setup in create_enclave()
	pml[0].pdp_base_addr = PAGE_TO_BASE_ADDR_4KB((u64)__pa(boot_params->level3_ident_pgt));
	pml[0].present = 1;
        pml[0].writable = 1;
        pml[0].accessed = 1;
        printk(KERN_DEBUG "Setup trampoline ident page table\n");


        // setup target address of linux trampoline
	// TODO: This should be the phys-addr of the launch code...
        trampoline_header->start = enclave->bootmem_addr_pa;
        printk(KERN_DEBUG "Setup trampoline target address 0x%p\n", (void *)trampoline_header->start);

        // wakeup CPU INIT/INIT/SINIT
        printk(KERN_INFO "PISCES: CPU%d (apic_id %d) wakeup CPU %u (apic_id %d) via INIT\n", 
                smp_processor_id(), 
                apic->cpu_present_to_apicid(smp_processor_id()), 
                enclave->boot_cpu, apicid);
        ret = wakeup_secondary_cpu_via_init(apicid, start_ip);

        // restore pml[0]
        pml[0] = tmp_pml0;

        mutex_unlock(cpu_maps_update_lock);

    }

    return ret;

}
