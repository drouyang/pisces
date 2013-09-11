

#include "pisces_loader.h"
#include "pisces_mod.h"
#include "domain_xcall.h"
#include "pgtables.h"
#include "boot_params.h"
#include "enclave.h"


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
    struct pisces_boot_params * boot_params = (struct pisces_boot_params *)__va(enclave->bootmem_addr_pa);
    printk(KERN_INFO "PISCES: launching kernel at  physical address %p\n", (void *)boot_params->kernel_addr);

    // rax = target cr3
    // rbx = kernel_addr
    // rcx = 2nd half of launch code
    // esi = base_addr

    __asm__ ( 
	     "jmp *%%rbx\n\t"
	     : 
	     : "a" (boot_params->ident_pgt_addr), 
	       "b" (boot_params->launch_code), 
	       "c" (boot_params->kernel_addr), 
	       "S" (enclave->bootmem_addr_pa)
	     : );

    // Will never get here
    return;
}
#endif


