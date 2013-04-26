#include <linux/fs.h>
#include<linux/percpu.h>
#include<asm/desc.h>
#include <asm/segment.h>
#include <asm/uaccess.h>
#include<asm/realmode.h>

#include"pisces_loader.h"
#include"pgtables_64.h"
#include"pisces.h"
#include"pisces_mod.h"
#include"domain_xcall.h"

#define ORDER 2
#define PAGE_SHIFT_2M 21


static bootstrap_pgt_t *bootstrap_pgt = NULL;
static struct pisces_mmap_t pisces_mmap;
static struct boot_params_t *boot_params;

extern int wakeup_secondary_cpu_via_init(int, unsigned long);

// setup bootstrap page tables - bootstrap_pgt
// 4M identity mapping from mem_base
void pgtable_setup_ident(unsigned long mem_base, unsigned long mem_len)
{

    u64 cr3;
    pgd64_t *pgd_base, *pgde;


    cr3 = get_cr3();

    pgd_base = (pgd64_t *) CR3_TO_PGD_VA(cr3);

    pgde = pgd_base + PGD_INDEX(mem_base);

    {
        int i;
        u64 tmp;

        bootstrap_pgt = (bootstrap_pgt_t *) __get_free_pages (GFP_KERNEL, ORDER);
        memset(bootstrap_pgt, 0, sizeof(bootstrap_pgt_t));
        /*
        printk("PISCES: level4_pgt va: 0x%lx, pa: 0x%lx\n", 
                (unsigned long) bootstrap_pgt->level4_pgt,
                __pa((unsigned long) bootstrap_pgt->level4_pgt));
        printk("PISCES: level3_pgt va: 0x%lx, pa: 0x%lx\n", 
                (unsigned long) bootstrap_pgt->level3_ident_pgt,
                __pa((unsigned long) bootstrap_pgt->level3_ident_pgt));
        printk("PISCES: level2_pgt va: 0x%lx, pa: 0x%lx\n", 
                (unsigned long) bootstrap_pgt->level2_ident_pgt,
                __pa((unsigned long) bootstrap_pgt->level2_ident_pgt));
        printk("PISCES: level1_pgt va: 0x%lx, pa: 0x%lx\n", 
                (unsigned long) bootstrap_pgt->level1_ident_pgt,
                __pa((unsigned long) bootstrap_pgt->level1_ident_pgt));
                */

        memcpy(bootstrap_pgt->level4_pgt, pgd_base, sizeof(NUM_PGD_ENTRIES*sizeof(u64)));

        //pgde->base_addr = PAGE_TO_BASE_ADDR(__pa(bootstrap_pgt->level3_ident_pgt));
        //pgde->present = 1; 
        //pgde->writable = 1; 
        //pgde->accessed = 1; 

        //bootstrap_pgt->level4_pgt[0] = *((pgd_2MB_t *)&level3_ident_pgt);
        tmp = PAGE_TO_BASE_ADDR(__pa(bootstrap_pgt->level3_ident_pgt));
        bootstrap_pgt->level4_pgt[PGD_INDEX(mem_base)].base_addr = tmp;
        bootstrap_pgt->level4_pgt[PGD_INDEX(mem_base)].present = 1; 
        bootstrap_pgt->level4_pgt[PGD_INDEX(mem_base)].writable = 1; 
        bootstrap_pgt->level4_pgt[PGD_INDEX(mem_base)].accessed = 1; 
        //printk("PISCES: level4[%llu].base_addr = %llx\n", PGD_INDEX(mem_base), tmp);

        tmp = PAGE_TO_BASE_ADDR(__pa(bootstrap_pgt->level2_ident_pgt));
        bootstrap_pgt->level3_ident_pgt[PUD_INDEX(mem_base)].base_addr =  tmp;
        bootstrap_pgt->level3_ident_pgt[PUD_INDEX(mem_base)].present =  1;
        bootstrap_pgt->level3_ident_pgt[PUD_INDEX(mem_base)].writable =  1;
        bootstrap_pgt->level3_ident_pgt[PUD_INDEX(mem_base)].accessed =  1;
        //printk("PISCES: level3[%llu].base_addr = %llx\n", PUD_INDEX(mem_base), tmp);

        tmp = PAGE_TO_BASE_ADDR(__pa(bootstrap_pgt->level1_ident_pgt));
        bootstrap_pgt->level2_ident_pgt[PMD_INDEX(mem_base)].base_addr =  tmp;
        bootstrap_pgt->level2_ident_pgt[PMD_INDEX(mem_base)].present =  1;
        bootstrap_pgt->level2_ident_pgt[PMD_INDEX(mem_base)].writable =  1;
        bootstrap_pgt->level2_ident_pgt[PMD_INDEX(mem_base)].accessed =  1;
        //printk("PISCES: level2[%llu].base_addr = %llx\n", PMD_INDEX(mem_base), tmp);

        for (i = 0; i < NUM_PTE_ENTRIES; i++) {
            bootstrap_pgt->level1_ident_pgt[i].base_addr = PAGE_TO_BASE_ADDR(mem_base + (i<<PAGE_POWER));
            bootstrap_pgt->level1_ident_pgt[i].present = 1;
            bootstrap_pgt->level1_ident_pgt[i].writable = 1;
            bootstrap_pgt->level1_ident_pgt[i].accessed = 1;
        }
        //printk("PISCES: level1[0].base_addr = %llx\n", (u64) bootstrap_pgt->level1_ident_pgt[0].base_addr);
        //printk("PISCES: level1[1].base_addr = %llx\n", (u64) bootstrap_pgt->level1_ident_pgt[1].base_addr);
        //printk("PISCES: cr3 va: 0x%lx\n", (unsigned long) pgd_base);
        //printk("PISCES: cr3[0].base_addr: 0x%lx\n", (unsigned long) pgd_base[0].base_addr);
    }

    return;
}

long load_image(char *path, unsigned long addr)
{
    struct file* filp = NULL;
    mm_segment_t oldfs;
    loff_t pos = 0;
    int err = 0;

    BUG_ON(path == NULL);

    // open file
    oldfs = get_fs();
    set_fs(get_ds());
    filp = filp_open(path, O_RDWR, 0);
    if(IS_ERR(filp)) {
        err = PTR_ERR(filp);
        return 0;
    }

    // read
    pos = 0;
    vfs_read(filp, __va(addr), 512 * 1024 * 1024, &pos);
    //printk(KERN_INFO "PISCES: read %ld bytes (%lu KB) from file\n", (long) pos, (long) pos / 1024);

    //printk(KERN_INFO "PISCES: %s", (char *)__va(addr));

    // close 
    set_fs(oldfs);
    filp_close(filp, NULL);
    return (long) pos;
}

void loader_exit(void) {
    free_pages((unsigned long)bootstrap_pgt, ORDER);
}

static struct pisces_mmap_t *mmap_init(void) 
{
    struct pisces_mmap_t *mmap = &pisces_mmap;
    int i;

    mmap->nr_map = 1;
    mmap->map[0].addr = mem_base;
    mmap->map[0].size = mem_len;

    printk(KERN_INFO "PISCES: offlined memory map:\n");
    for(i=0; i<mmap->nr_map; i++) {
        printk(KERN_INFO "  %d: [0x%llx, 0x%llx),  size 0x%llx\n",
                i,
                mmap->map[i].addr,
                mmap->map[i].addr+mmap->map[i].size,
                mmap->map[i].size);
    }

    return mmap;
}

/* loader memory layout
 * 1. kernel image // 2M aligned
 * 2. initrd // 2M aligned
 * 2. shared_info
 *
 */
extern char *kernel_path;
extern char *initrd_path;
static void mem_layout_init(void) 
{
    long mem_base;
    long base;
    long size = 0;
    struct pisces_mmap_t *mmap; 
    struct boot_params_t local_boot_params;

    mmap = mmap_init();

    BUG_ON(mmap->nr_map < 1);

    mem_base = mmap->map[0].addr;
    base = mem_base;

    // 1. kernel image
    size = load_image(kernel_path, base);
    local_boot_params.kernel_addr = base;
    local_boot_params.kernel_size = size;


    
    /* 2M memory gap between kernel and initrd*/

    // 2. initrd 
    base = mem_base + (512<<PAGE_SHIFT); // 512*4k = 2M aligned
    size = load_image(initrd_path, base);
    local_boot_params.initrd_addr = base;
    local_boot_params.initrd_size = size;


    // 3. shared_info load to next page
    base += (((size>>PAGE_SHIFT)+1)<<PAGE_SHIFT); //4K roundup
    shared_info = (struct shared_info_t *)__va(base);
    size = sizeof(struct shared_info_t);
    local_boot_params.shared_info_addr = base;
    local_boot_params.shared_info_size = size;


    // copy boot params to offlined memory
    memset((void *)shared_info, 0, sizeof(struct shared_info_t));
    shared_info->magic = PISCES_MAGIC;
    boot_params = &shared_info->boot_params;
    memcpy(boot_params, &local_boot_params, sizeof(struct boot_params_t));

    memcpy(&boot_params->mmap, mmap, sizeof(struct pisces_mmap_t));

    // boot command line
    strcpy(boot_params->cmd_line, boot_cmd_line);

    printk(KERN_INFO "PISCES: loader memroy map:\n");
    printk(KERN_INFO "  kernel:        [0x%lx, 0x%lx), size 0x%lx\n",
            boot_params->kernel_addr,
            boot_params->kernel_addr+boot_params->kernel_size,
            boot_params->kernel_size);
    printk(KERN_INFO "  initrd:        [0x%lx, 0x%lx), size 0x%lx\n",
            boot_params->initrd_addr, 
            boot_params->initrd_addr+boot_params->initrd_size,
            boot_params->initrd_size);
    printk(KERN_INFO "  shared_info:   [0x%lx, 0x%lx), size 0x%lx\n",
            boot_params->shared_info_addr, 
            boot_params->shared_info_addr+boot_params->shared_info_size,
            boot_params->shared_info_size);

}

static void pisces_trampoline(void)
{
    unsigned long pgd_phys = __pa((unsigned long)bootstrap_pgt->level4_pgt);
    unsigned long target = (unsigned long)(boot_params->kernel_addr);
    int magic = PISCES_MAGIC;

    printk(KERN_INFO "PISCES: jump to physical address 0x%lx\n", target);
    __asm__ ( "movq %0, %%rax\n\t"
            "movq %%rax, %%cr3\n\t" //cr3
            "movl %2, %%esi\n\t" // PISCES_MAGIC
            "movq %1, %%rax\n\t" //target
            "jmp *%%rax\n\t" // should never return
            "hlt\n\t"
            : 
            : "r" (pgd_phys), "r" (target), "r" ((unsigned int)boot_params->shared_info_addr | magic)
            : "%rax", "%esi");
}

// use linux trampoline code to init offlined cpu, but hijack and jump to pisces_trampoline
// in pisces_trampoline, setup ident mapping and jump to kernel code
int kick_offline_cpu(void) 
{
    int ret = 0;
    int apicid = apic->cpu_present_to_apicid(cpu_id);
    unsigned long start_ip = real_mode_header->trampoline_start;

    // gdt for the kernel to access user space memory
    early_gdt_descr.address = (unsigned long)per_cpu(gdt_page, cpu_id).gdt;

    // setup ident mapping for pisces_trampoline
    pgtable_setup_ident(mem_base, mem_len);

    // our pisces_trampoline
    initial_code = (unsigned long) pisces_trampoline;

    printk(KERN_INFO "PISCES: CPU%d (apic_id %d) wakeup CPU%lu (apic_id %d) via INIT\n", smp_processor_id(), apic->cpu_present_to_apicid(smp_processor_id()), cpu_id, apicid);
    ret = wakeup_secondary_cpu_via_init(apicid, start_ip);
    return ret;

}


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
    mpc = (struct mpc_table *)phys_to_virt(mpf->physptr);
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
	int count=sizeof(*mpc);
	unsigned char *mpt=((unsigned char *)mpc)+count;

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
    fix_mpc(&boot_params->mpc);

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

void start_instance(void)
{
    // memory init goes first because we need to setup share_info
    // region first
    mem_layout_init();

    if (cpu_info_init() >= 0) {
        kick_offline_cpu();
    }

}
