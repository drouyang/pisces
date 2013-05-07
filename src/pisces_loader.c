#include <linux/fs.h>
#include <linux/mutex.h>
#include <linux/percpu.h>
#include <linux/kallsyms.h>
#include <asm/desc.h>
#include <asm/segment.h>
#include <asm/uaccess.h>
#include <asm/realmode.h>

#include "pisces_loader.h"
#include "pisces.h"
#include "pisces_mod.h"
#include "domain_xcall.h"
#include "pgtables.h"

extern int wakeup_secondary_cpu_via_init(int, unsigned long);

#if 0
static int mpf_check(void)
{
    unsigned long mpf_found = *(unsigned long *) mpf_found_addr;
    struct pisces_mpf_intel *mpf = (struct pisces_mpf_intel *)mpf_found;
    struct mpc_table *mpc = NULL;
    char *str = (char *) mpf_found;

    if (!mpf) {
        printk(KERN_INFO "PISCES: No MP Table found.\n");
        return -1;
    }

    if (str[0]!='_' || str[1]!='M' || str[2]!='P' || str[3]!='_') {
        printk("PISCES: wrong mpf signiture \"%c%c%c%c\"\n", str[0], str[1], str[2], str[3]);
        return -1;
    }

    if (mpf->feature1 != 0)
        return -1;
    if (!mpf->physptr)
        return -1;

    /*
     * Parse the MP configuration
     */
    mpc = (struct mpc_table *)__va(mpf->physptr);
    if (memcmp(mpc->signature, MPC_SIGNATURE, 4)) {
        printk(KERN_INFO "PISCES: wrong mpc signiture");
        return -1;
    }
    return 0;
}


static unsigned char cal_checksum(char *s, int len)
{
        unsigned char checksum = 0;

        while (len--)
            checksum -= *s++;

        return checksum;
}

/**
 * Parses the input MP table, modify for Pisces needs
 */
static void fix_mpc(struct pisces_mpc_table *mpc)
{

}




/*
 * setup the MP table by modify from the linux copy
 */
static int mpf_setup(void)
{
    unsigned long mpf_found = *(unsigned long *)mpf_found_addr;
    struct pisces_mpf_intel *mpf = (struct pisces_mpf_intel *)mpf_found; 

    printk(KERN_INFO "PISCES: setup MP tables\n"); 

    if (mpf_check() < 0) {
        return -1;
    }
    // copy mpf header
    memcpy(&boot_params->mpf, 
            mpf, 
            sizeof(struct pisces_mpf_intel)); 
    // copy mpc header
    memcpy(&boot_params->mpc, 
            __va(mpf->physptr), 
            sizeof(struct pisces_mpc_table));
    // copy mpc
    memcpy(&boot_params->mpc, 
            __va(mpf->physptr), 
            boot_params->mpc.length); 

    // fix mpc address in mpf and checksum
    boot_params->mpf.physptr = __pa(&boot_params->mpc);
    boot_params->mpf.checksum = 0;
    boot_params->mpf.checksum = cal_checksum((unsigned char *)&boot_params->mpf, 16);

    // modify MP table to fit Pisces


    {
	int count = sizeof(struct pisces_mpc_table);
	unsigned char * mpt = ((unsigned char *)mpc) + count;

	/* Now process all of the configuration blocks in the table. */
	while (count < mpc->length) {
		switch(*mpt) {
			case PISCES_MP_PROCESSOR:
			{
				struct pisces_mpc_processor *m=
					(struct pisces_mpc_processor *)mpt;
                                // JO: hardcode BSP and available cpus 
                                if (m->apicid != 1 && m->apicid != 2) {
                                    m->cpuflag &= ~PISCES_CPU_ENABLED;
                                    m->cpuflag &= ~PISCES_CPU_BSP;
                                    printk(KERN_INFO "  disable cpu apicid %d\n", m->apicid);
                                } else if (m->apicid == 1){
                                    m->cpuflag |= PISCES_CPU_BSP;
                                    printk(KERN_INFO "  enable  cpu apicid %d (BSP)\n", m->apicid);
                                } else {
                                    printk(KERN_INFO "  enable  cpu apicid %d\n", m->apicid);
                                }

				mpt += sizeof(*m);
				count += sizeof(*m);
				break;
			}
                        default:
                        {
                            mpc->length = count;
                            break;
                        }
		}
	}

        // fix checksum
        boot_params->mpc.checksum = 0;
        boot_params->mpc.checksum = 
            cal_checksum((unsigned char *)&boot_params->mpc, boot_params->mpc.length);

    }

    return 0;
}



static int cpu_info_init(void)
{
    int apicid = apic->cpu_present_to_apicid(smp_processor_id());

    // cpu map

    // domain_xcall
    boot_params->domain_xcall_master_apicid = apicid;
    boot_params->domain_xcall_vector = vector;

    if (mpf_setup() < 0) {
        printk(KERN_INFO "PISCES: start instance failed.");
        return -1;
    }

    return 0;
}


static void pisces_trampoline(struct pisces_enclave * enclave) {  
    struct pisces_boot_params * boot_params = (struct pisces_boot_params *)__va(enclave->base_addr_pa);
    printk(KERN_INFO "PISCES: launching kernel at  physical address %p\n", (void *)boot_params->kernel_addr);

    // rax = target cr3
    // rbx = kernel_addr
    // rcx = 2nd half of launch code
    // esi =  PISCES_MAGIC + base_addr

    __asm__ ( 
	     "jmp *%%rbx\n\t"
	     : 
	     : "a" (boot_params->ident_pgt_addr), 
	       "b" (boot_params->launch_code), 
	       "c" (boot_params->kernel_addr), 
	       "S" (enclave->base_addr_pa | PISCES_MAGIC)
	     : );

    // Will never get here
    return;
}
#endif



// use linux trampoline code to init offlined cpu, but hijack and jump to pisces_trampoline
// in pisces_trampoline, setup ident mapping and jump to kernel code
int kick_offline_cpu(struct pisces_enclave * enclave) 
{

    int ret = 0;
    int apicid = apic->cpu_present_to_apicid(cpu_id);
    struct pisces_boot_params * boot_params = (struct pisces_boot_params *)__va(enclave->base_addr_pa);

    printk(KERN_DEBUG "Boot Pisces guest cpu\n");

    // patch launch_code
    {
	extern u8 launch_code_start;
        extern u64 launch_code_target_addr;
        extern u64 launch_code_esi;
        u64 *target_addr_ptr = NULL;
        u64 *esi_ptr = NULL;

        // setup launch code data
        target_addr_ptr =  (u64 *)((u8 *)&boot_params->launch_code 
            + ((u8 *)&launch_code_target_addr - (u8 *)&launch_code_start));
        esi_ptr =  (u64 *)((u8 *)&boot_params->launch_code 
            + ((u8 *)&launch_code_esi - (u8 *)&launch_code_start));
        *target_addr_ptr = boot_params->kernel_addr;
        *esi_ptr = enclave->base_addr_pa | PISCES_MAGIC;
        printk(KERN_DEBUG "  patch target address 0x%p at 0x%p\n", 
                (void *) *target_addr_ptr, (void *) __pa(target_addr_ptr));
        printk(KERN_DEBUG "  patch esi 0x%p at 0x%p\n", 
                (void *) *esi_ptr, (void *) __pa(esi_ptr));
    }

    // setup linux trampoline
    {
        u64 header_addr = kallsyms_lookup_name("real_mode_header");
        struct real_mode_header * real_mode_header = *(struct real_mode_header **)header_addr;
        u64 start_ip = real_mode_header->trampoline_start;
        struct trampoline_header * trampoline_header = (struct trampoline_header *) __va(real_mode_header->trampoline_header);
        pml4e64_t * pml = (pml4e64_t *) __va(real_mode_header->trampoline_pgd);
        pml4e64_t tmp_pml0;

        u64 cpu_maps_update_lock_addr =  kallsyms_lookup_name("cpu_add_remove_lock");
        struct mutex * cpu_maps_update_lock = (struct mutex *)cpu_maps_update_lock_addr;

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
        trampoline_header->start = enclave->base_addr_pa;
        printk(KERN_DEBUG "Setup trampoline target address 0x%p\n", (void *)trampoline_header->start);

        // wakeup CPU INIT/INIT/SINIT
        printk(KERN_INFO "PISCES: CPU%d (apic_id %d) wakeup CPU%lu (apic_id %d) via INIT\n", 
                smp_processor_id(), 
                apic->cpu_present_to_apicid(smp_processor_id()), 
                cpu_id, apicid);
        ret = wakeup_secondary_cpu_via_init(apicid, start_ip);

        // restore pml[0]
        pml[0] = tmp_pml0;

        mutex_unlock(cpu_maps_update_lock);

    }

    return ret;

}



int launch_enclave(struct pisces_enclave * enclave) {

//  if (cpu_info_init() >= 0) {
    kick_offline_cpu(enclave);
//    }

    return 0;
}
