#include <linux/init.h>
#include <linux/smp.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/percpu.h>
#include <linux/bootmem.h>
#include <linux/err.h>
#include <linux/nmi.h>
#include <linux/tboot.h>
#include <linux/stackprotector.h>
#include <linux/gfp.h>
#include <linux/cpuidle.h>

#include <asm/acpi.h>
#include <asm/desc.h>
#include <asm/nmi.h>
#include <asm/irq.h>
#include <asm/idle.h>
#include <asm/cpu.h>
#include <asm/numa.h>
#include <asm/pgtable.h>
#include <asm/tlbflush.h>
#include <asm/mtrr.h>
#include <asm/apic.h>
#include <asm/io_apic.h>
#include <asm/i387.h>
#include <asm/setup.h>
#include <asm/uv/uv.h>
#include <linux/mc146818rtc.h>

#include <asm/smpboot_hooks.h>
#include <asm/i8259.h>

#include "linux_syms.h"
#include "enclave.h"
#include "pisces_boot_params.h"

#include "pgtables.h"
#include "linux_syms.h"


#include "boot.h"


#ifdef CRAY_TRAMPOLINE
#include "cray_trampoline/trampoline.h"
#else
#include "linux_trampoline/trampoline.h"
#endif


struct trampoline_data trampoline_state;


static struct page * pgt_pages = NULL;


static void
init_trampoline_pgts(void)
{
    /* 
     * Clear the pages individually to avoid any overrun bugs
     */
    memset(__va(trampoline_state.pml_pa),  0, PAGE_SIZE);
    memset(__va(trampoline_state.pdp0_pa), 0, PAGE_SIZE);
    memset(__va(trampoline_state.pdp1_pa), 0, PAGE_SIZE);
    memset(__va(trampoline_state.pdp2_pa), 0, PAGE_SIZE);
    memset(__va(trampoline_state.pd0_pa),  0, PAGE_SIZE);
    memset(__va(trampoline_state.pd1_pa),  0, PAGE_SIZE);
    memset(__va(trampoline_state.pd2_pa),  0, PAGE_SIZE);



    /* Map only first 2MB as identity  */
    {
	pml4e64_t   * pml  = __va(trampoline_state.pml_pa);
	pdpe64_t    * pdp0 = __va(trampoline_state.pdp0_pa);
	pde64_2MB_t * pd0  = __va(trampoline_state.pd0_pa);

	pml4e64_t   * pml_entry = &(pml[0]);
	pdpe64_t    * pdp_entry = &(pdp0[0]);
	pde64_2MB_t * pd_entry  = &(pd0[0]);

	pml_entry->present       = 1;
	pml_entry->writable      = 1;
	pml_entry->pdp_base_addr = PAGE_TO_BASE_ADDR(trampoline_state.pdp0_pa);

	pdp_entry->present       = 1;
	pdp_entry->writable      = 1;
	pdp_entry->pd_base_addr  = PAGE_TO_BASE_ADDR(trampoline_state.pd0_pa);

	pd_entry->present        = 1;
	pd_entry->writable       = 1;
	pd_entry->large_page     = 1;
	pd_entry->page_base_addr = PAGE_TO_BASE_ADDR_2MB(0);

    }
}


int 
pisces_init_trampoline(void)
{


    memset(&trampoline_state, 0, sizeof(struct trampoline_data));

    pgt_pages = alloc_pages(GFP_KERNEL, get_order(PAGE_SIZE * 7));

    if (pgt_pages == NULL) {
	printk(KERN_ERR "Error: Could not allocate pages for trampoline page table\n");
	return -1;
    }

    trampoline_state.pml_pa  = (page_to_pfn(&(pgt_pages[0])) << PAGE_SHIFT);
    trampoline_state.pdp0_pa = (page_to_pfn(&(pgt_pages[1])) << PAGE_SHIFT);
    trampoline_state.pdp1_pa = (page_to_pfn(&(pgt_pages[2])) << PAGE_SHIFT);
    trampoline_state.pdp2_pa = (page_to_pfn(&(pgt_pages[3])) << PAGE_SHIFT);
    trampoline_state.pd0_pa  = (page_to_pfn(&(pgt_pages[4])) << PAGE_SHIFT);
    trampoline_state.pd1_pa  = (page_to_pfn(&(pgt_pages[5])) << PAGE_SHIFT);
    trampoline_state.pd2_pa  = (page_to_pfn(&(pgt_pages[6])) << PAGE_SHIFT);

    /*
     * Init trampoline PGTs 
     */
    init_trampoline_pgts();
#ifdef CRAY_TRAMPOLINE
    return init_cray_trampoline();
#else
    return init_linux_trampoline();
#endif
}




int 
pisces_setup_trampoline(struct pisces_enclave * enclave) 
{


    if (enclave->bootmem_size % PAGE_SIZE_2MB) {
	printk(KERN_ERR "Error: Bootmem must be at least 2MB granularity\n");
	return -1;
    }


    /*
     * Setup trampoline PGTs 
     *  --  re-init pgts
     *  --  identity map first 128MB of bootmem 
     *  --  map 128MB of bootmem into kernel addresses 
     */
    init_trampoline_pgts();

    {
	pml4e64_t   * pml_entry = NULL;
	pdpe64_t    * pdp_entry = NULL;
	pde64_2MB_t * pd_entry  = NULL;

	pml4e64_t   * pml = NULL;
	pdpe64_t    * pdp = NULL;
	pde64_2MB_t * pd  = NULL;
	
	uintptr_t base_addr = enclave->bootmem_addr_pa;
	u32 num_pde_entries = (enclave->bootmem_size / PAGE_SIZE_2MB);
	
	int i = 0;


	pml       = (pml4e64_t  *)__va(trampoline_state.pml_pa);
	pml_entry = &(pml[PML4E64_INDEX(base_addr)]);


	if (!pml_entry->present) {
	    pml_entry->present       = 1;
	    pml_entry->writable      = 1;
	    pml_entry->pdp_base_addr = PAGE_TO_BASE_ADDR(trampoline_state.pdp1_pa);
	} 

	pdp       = __va(BASE_TO_PAGE_ADDR(pml_entry->pdp_base_addr));
	pdp_entry = &(pdp[PDPE64_INDEX(base_addr)]);


	if (!pdp_entry->present) {
	    pdp_entry->present      = 1;
	    pdp_entry->writable     = 1;
	    pdp_entry->pd_base_addr = PAGE_TO_BASE_ADDR(trampoline_state.pd1_pa);
	}

	pd  = __va(BASE_TO_PAGE_ADDR(pdp_entry->pd_base_addr));


	for (i = 0; i < num_pde_entries; i++) {
	    pd_entry = &(pd[PDE64_INDEX(base_addr)]);

	    if (!pd_entry->present) {
		pd_entry->present    = 1;
		pd_entry->writable   = 1;
		pd_entry->large_page = 1;
		pd_entry->page_base_addr = PAGE_TO_BASE_ADDR_2MB(base_addr);
	    }

	    base_addr += PAGE_SIZE_2MB;
	}
	
    }



    {
#define KERN_VA_BASE 0xffffffff80000000ULL

	pml4e64_t   * pml_entry = NULL;
	pdpe64_t    * pdp_entry = NULL;
	pde64_2MB_t * pd_entry  = NULL;

	pml4e64_t   * pml = NULL;
	pdpe64_t    * pdp = NULL;
	pde64_2MB_t * pd  = NULL;
	
	uintptr_t kern_addr    = KERN_VA_BASE;
	uintptr_t bootmem_addr = enclave->bootmem_addr_pa;
	u32 num_pde_entries    = (enclave->bootmem_size / PAGE_SIZE_2MB);
	
	int i = 0;

	pml       = (pml4e64_t  *)__va(trampoline_state.pml_pa);
	pml_entry = &(pml[PML4E64_INDEX(kern_addr)]);


	if (!pml_entry->present) {
	    pml_entry->present       = 1;
	    pml_entry->writable      = 1;
	    pml_entry->pdp_base_addr = PAGE_TO_BASE_ADDR(trampoline_state.pdp1_pa);
	} 

	pdp       = __va(BASE_TO_PAGE_ADDR(pml_entry->pdp_base_addr));
	pdp_entry = &(pdp[PDPE64_INDEX(kern_addr)]);


	if (!pdp_entry->present) {
	    pdp_entry->present      = 1;
	    pdp_entry->writable     = 1;
	    pdp_entry->pd_base_addr = PAGE_TO_BASE_ADDR(trampoline_state.pd1_pa);
	}

	pd  = __va(BASE_TO_PAGE_ADDR(pdp_entry->pd_base_addr));


	for (i = 0; i < num_pde_entries; i++) {
	    pd_entry = &(pd[PDE64_INDEX(kern_addr)]);

	    if (!pd_entry->present) {
		pd_entry->present    = 1;
		pd_entry->writable   = 1;
		pd_entry->large_page = 1;
		pd_entry->page_base_addr = PAGE_TO_BASE_ADDR_2MB(bootmem_addr);
	    }

	    kern_addr    += PAGE_SIZE_2MB;
	    bootmem_addr += PAGE_SIZE_2MB;
	}

    }

    // walk_pgtables((uintptr_t)__va(trampoline_state.pml_pa));

#ifdef CRAY_TRAMPOLINE
    return setup_cray_trampoline(enclave);
#else 
    return setup_linux_trampoline(enclave);
#endif
}


int
pisces_restore_trampoline(struct pisces_enclave * enclave) 
{
#ifdef CRAY_TRAMPOLINE
    return restore_cray_trampoline(enclave);    
#else
    return restore_linux_trampoline(enclave);
#endif
}


/*
 * Update Pisces trampoline data
 */
static void
set_enclave_launch_args(struct pisces_enclave * enclave, 
			u64                     target_addr, 
			u64                     esi)
{
    struct pisces_boot_params * boot_params = (struct pisces_boot_params *)__va(enclave->bootmem_addr_pa);

    printk("Setup Enclave trampoline\n");

    boot_params->launch_code_esi         = esi;
    boot_params->launch_code_target_addr = target_addr;

    printk(KERN_DEBUG "  set target address at %p to %p\n", 
	   (void *) __pa(&(boot_params->launch_code_target_addr)), 
	   (void *) boot_params->launch_code_target_addr);

    printk(KERN_DEBUG "  set esi value at %p to %p\n", 
	   (void *) __pa(&(boot_params->launch_code_esi)), 
	   (void *) boot_params->launch_code_esi);
}




static int 
__lapic_get_maxlvt(void)
{
    unsigned int v;

    v = apic_read(APIC_LVR);

    return APIC_INTEGRATED(GET_APIC_VERSION(v)) ? GET_APIC_MAXLVT(v) : 2;
}



static int 
__wakeup_secondary_cpu_via_init(int phys_apicid)
{
        unsigned long send_status, accept_status = 0;
        int maxlvt, num_starts, j;

        maxlvt = __lapic_get_maxlvt();

        /*
         * Be paranoid about clearing APIC errors.
         */
        if (APIC_INTEGRATED(apic_version[phys_apicid])) {
                if (maxlvt > 3)         /* Due to the Pentium erratum 3AP.  */
                        apic_write(APIC_ESR, 0);
                apic_read(APIC_ESR);
        }

        pr_debug("Asserting INIT\n");

        /*
         * Turn INIT on target chip
         */
        /*
         * Send IPI
         */
        apic_icr_write(APIC_INT_LEVELTRIG | APIC_INT_ASSERT | APIC_DM_INIT,
                       phys_apicid);

        pr_debug("Waiting for send to finish...\n");
        send_status = safe_apic_wait_icr_idle();

        mdelay(10);

        pr_debug("Deasserting INIT\n");

        /* Target chip */
        /* Send IPI */
        apic_icr_write(APIC_INT_LEVELTRIG | APIC_DM_INIT, phys_apicid);

        pr_debug("Waiting for send to finish...\n");
        send_status = safe_apic_wait_icr_idle();

        mb();
        //atomic_set(&init_deasserted, 1);

        /*
         * Should we send STARTUP IPIs ?
         *
         * Determine this based on the APIC version.
         * if we don't have an integrated APIC, don't send the STARTUP IPIs.
         */
        if (APIC_INTEGRATED(apic_version[phys_apicid]))
                num_starts = 2;
        else
                num_starts = 0;

        /*
         * Paravirt / VMI wants a startup IPI hook here to set up the
         * target processor state.
         */
#if 0

	/* JRL: Disabled for now
	 *  --  They need to be set to the enclave ptrs if we use them
	 */
        startup_ipi_hook(phys_apicid, (unsigned long) linux_start_secondary_addr,
                         linux_stack_start);

#endif
        /*
         * Run STARTUP IPI loop.
         */
        pr_debug("#startup loops: %d\n", num_starts);

        for (j = 1; j <= num_starts; j++) {
                pr_debug("Sending STARTUP #%d\n", j);
                if (maxlvt > 3)         /* Due to the Pentium erratum 3AP.  */
                        apic_write(APIC_ESR, 0);
                apic_read(APIC_ESR);
                pr_debug("After apic_write\n");

                /*
                 * STARTUP IPI
                 */

                /* Target chip */
                /* Boot on the stack */
                /* Kick the second */
                apic_icr_write(APIC_DM_STARTUP | (trampoline_state.cpu_init_rip >> 12),
                               phys_apicid);

                /*
                 * Give the other CPU some time to accept the IPI.
                 */
                udelay(300);

                pr_debug("Startup point 1\n");

                pr_debug("Waiting for send to finish...\n");
                send_status = safe_apic_wait_icr_idle();

                /*
                 * Give the other CPU some time to accept the IPI.
                 */
                udelay(200);
                if (maxlvt > 3)         /* Due to the Pentium erratum 3AP.  */
                       apic_write(APIC_ESR, 0);
                accept_status = (apic_read(APIC_ESR) & 0xEF);
                if (send_status || accept_status)
                        break;
        }
        pr_debug("After Startup\n");

        if (send_status)
                pr_err("APIC never delivered???\n");
        if (accept_status)
                pr_err("APIC delivery error (%lx)\n", accept_status);

        return (send_status | accept_status);
}




int 
boot_enclave(struct pisces_enclave * enclave) 
{
    struct pisces_boot_params * boot_params = (struct pisces_boot_params *)__va(enclave->bootmem_addr_pa);

    int apicid = apic->cpu_present_to_apicid(enclave->boot_cpu);
    int ret    = 0;

    printk(KERN_DEBUG "Boot Enclave on CPU %d (APIC=%d)...\n", 
	   enclave->boot_cpu, apicid);

    set_enclave_launch_args(enclave,
			    boot_params->kernel_addr,
			    enclave->bootmem_addr_pa >> PAGE_SHIFT);



    if (pisces_setup_trampoline(enclave) != 0) {
	return -1;
    }
	
	
    printk(KERN_INFO "Reset APIC %d from APIC %d (CPU=%d)\n", 
	   apicid,
	   apic->cpu_present_to_apicid(smp_processor_id()), smp_processor_id());
	
    __wakeup_secondary_cpu_via_init(apicid);
	
    /* Delay for target CPU to use Linux trampoline*/
    //udelay(500);
    mdelay(10);
	
    pisces_restore_trampoline(enclave);


    return ret;
}
