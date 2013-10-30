#include <linux/fs.h>
#include <linux/mutex.h>
#include <linux/percpu.h>

#include <asm/desc.h>
#include <asm/segment.h>
#include <asm/uaccess.h>
#include <asm/realmode.h>

#include "pisces_boot_params.h"
#include "enclave.h"
#include "file_io.h"
#include "pisces_ringbuf.h"
#include "pisces_ctrl.h"

#include "linux_syms.h"

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
        extern u8 launch_code_start;

        memcpy(boot_params->launch_code, &launch_code_start, LAUNCH_CODE_SIZE);

        printk("Launch code is at 0x%p\n",
                (void *)__pa(&boot_params->launch_code));
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

        // Linux trampoline address
	boot_params->trampoline_code_pa = linux_trampoline_startip;
	printk("Linux trampoline at %p\n", (void *)linux_trampoline_startip);

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


/*
 * Update Pisces trampoline data
 */
static void
set_enclave_trampoline(
        struct pisces_enclave *enclave, 
        u64 target_addr, 
        u64 esi)
{
    extern u8 launch_code_start;

    extern u64 launch_code_target_addr;
    extern u64 launch_code_esi;

    u64 * target_addr_ptr = NULL;
    u64 * esi_ptr = NULL;
    struct pisces_boot_params * boot_params = 
        (struct pisces_boot_params *)__va(enclave->bootmem_addr_pa);

    printk("Setup Enclave trampoline\n");

    target_addr_ptr =  (u64 *)((u8 *)&boot_params->launch_code 
            + ((u8 *)&launch_code_target_addr - (u8 *)&launch_code_start));
    esi_ptr =  (u64 *)((u8 *)&boot_params->launch_code 
            + ((u8 *)&launch_code_esi - (u8 *)&launch_code_start));

    *target_addr_ptr = target_addr;
    *esi_ptr = esi;

    printk(KERN_DEBUG "  set target address at 0x%p to 0x%p\n", 
            (void *) __pa(target_addr_ptr), (void *) *target_addr_ptr);
    printk(KERN_DEBUG "  set esi value at 0x%p to 0x%p\n", 
            (void *) __pa(esi_ptr), (void *) *esi_ptr);
}


inline void setup_linux_trampoline_target(u64 target_addr)
{
    *linux_trampoline_target = target_addr;
    printk(KERN_DEBUG "Setup linux trampoline target address 0x%p\n", 
            (void *)target_addr);
}

/*
 * Reset CPU with INIT/INIT/SINIT IPIs
 */
inline void reset_cpu(int apicid)
{
    printk(KERN_INFO "Reset CPU %d from CPU %d\n", 
            apicid,
            apic->cpu_present_to_apicid(smp_processor_id()));
    wakeup_secondary_cpu_via_init(apicid, linux_trampoline_startip);
}
/*
 * Setup identity mapped page table for Linux trampoline
 * default only maps first 1G, this function setup mapping
 * for enclave boot memory
 *
 * TODO: restore modification afterwards
 */
void setup_linux_trampoline_pgd(u64 target_addr)
{

    // use the level3_ident_pgt setup in create_enclave()
    // pml[0].pdp_base_addr = PAGE_TO_BASE_ADDR_4KB((u64)__pa(boot_params->level3_ident_pgt));
    // pml[0].present = 1;
    // pml[0].writable = 1;
    // pml[0].accessed = 1;
    // printk(KERN_DEBUG "Setup trampoline ident page table\n");
}

void trampoline_lock(void) {
    mutex_lock(linux_trampoline_lock);
}

void trampoline_unlock(void) {
    mutex_unlock(linux_trampoline_lock);
}

int boot_enclave(struct pisces_enclave * enclave) 
{
    int ret = 0;
    int apicid = apic->cpu_present_to_apicid(enclave->boot_cpu);
    struct pisces_boot_params * boot_params = (struct pisces_boot_params *)__va(enclave->bootmem_addr_pa);

    printk(KERN_DEBUG "Boot Enclave...\n");

    set_enclave_trampoline(enclave,
            boot_params->kernel_addr,
            enclave->bootmem_addr_pa >> PAGE_SHIFT);
    /*
     * hold this lock to serialize trampoline data access 
     * as cpu_maps_update_begin in linux
     */
    mutex_lock(linux_trampoline_lock);
    setup_linux_trampoline_pgd(enclave->bootmem_addr_pa);
    setup_linux_trampoline_target(enclave->bootmem_addr_pa);
    reset_cpu(apicid);
    /* Delay for target CPU to use Linux trampoline*/
    udelay(500);
    mutex_unlock(linux_trampoline_lock);

    return ret;
}
