#include <linux/fs.h>
//#include <linux/buffer_head.h>
#include<linux/percpu.h>
#include<asm/desc.h>
#include <asm/segment.h>
#include <asm/uaccess.h>
#include<asm/realmode.h>

#include"pisces_loader.h"
#include"pgtables_64.h"
#include"pisces.h"
#include"pisces_mod.h"

#define ORDER 2
#define PAGE_SHIFT_2M 21


static bootstrap_pgt_t *bootstrap_pgt = NULL;
static struct pisces_mmap_t pisces_mmap;
static struct boot_params_t *boot_params;
static struct pisces_mpf_intel *mpf;

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

/* loader memory layout
 * 1. shared_info
 * 2. initrd
 * 3. kernel image // 2M aligned
 *
 */
extern char *kernel_path;
extern char *initrd_path;
struct boot_params_t *setup_memory_layout(struct pisces_mmap_t *mmap) 
{
    long mem_base = mmap->map[0].addr;
    long base = mem_base;
    long size = 0;
    long offset = 0;
    struct shared_info_t *shared_info;
    struct boot_params_t *boot_params;
    struct boot_params_t local_boot_params;

    BUG_ON(mmap->nr_map < 1);

    // 1. kernel image
    size = load_image(kernel_path, base);
    local_boot_params.kernel_addr = base;
    local_boot_params.kernel_size = size;


    
    /* 2M memory gap between kernel and initrd*/

    // 2. initrd 
    base = mem_base + (512<<PAGE_SHIFT); //roundup
    size = load_image(initrd_path, base);
    local_boot_params.initrd_addr = base;
    local_boot_params.initrd_size = size;


    // 3. shared_info load to next page
    base += (((size>>PAGE_SHIFT)+1)<<PAGE_SHIFT); //4K roundup
    shared_info = (struct shared_info_t *)__va(base);
    size = sizeof(struct shared_info_t);
    local_boot_params.shared_info_addr = base;
    local_boot_params.shared_info_size = offset;


    // copy boot params to offlined memory
    memset((void *)shared_info, 0, sizeof(struct shared_info_t));
    shared_info->magic = PISCES_MAGIC;
    boot_params = &shared_info->boot_params;
    memcpy(boot_params, &local_boot_params, sizeof(struct boot_params_t));

    memcpy(&boot_params->mmap, mmap, sizeof(struct pisces_mmap_t));

    printk(KERN_INFO "PISCES: guest image loaded\n");
#if 0
    printk(KERN_INFO "PISCES: kernel base 0x%lx size %ld\n", base, size);
    printk(KERN_INFO "PISCES: initrd base 0x%lx size %ld\n", base, size);
    printk(KERN_INFO "PISCES: shared_info base 0x%lx size %ld\n", base, size);
    printk(KERN_INFO "PISCES: mmap[0] base 0x%llx size 0x%llx\n", boot_params->mmap.map[0].addr, boot_params->mmap.map[0].size);
#endif

    return boot_params;
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
void start_instance(void)
{
        struct pisces_mmap_t *mmap = &pisces_mmap;

        // 0. setup offlined memory map
        mmap->nr_map = 1;
        mmap->map[0].addr = mem_base;
        mmap->map[0].size = mem_len;

        // 1. load images, setup loader memory layout
        boot_params = setup_memory_layout(mmap);
        strcpy(boot_params->cmd_line, boot_cmd_line);
        shared_info = container_of(boot_params, struct shared_info_t, boot_params);
        
#if 0
        // check and compare with original binary file using xxd
        printk(KERN_INFO "PISCES: boot command line: %s\n", boot_params->cmd_line); 
        printk(KERN_INFO "PISCES: kernel image header 0x%lx: 0x%lx\n", 
                boot_params->kernel_addr, *(unsigned long *)__va(boot_params->kernel_addr));
        printk(KERN_INFO "PISCES: initrd image header 0x%lx: 0x%lx\n", 
                boot_params->initrd_addr, *(unsigned long *)__va(boot_params->initrd_addr));
#endif

        // 2. setup MP tables
        memcpy(&boot_params->mpf, mpf, sizeof(struct pisces_mpf_intel));
        memcpy(&boot_params->mpc, __va(mpf->physptr), sizeof(struct pisces_mpc_table));
        memcpy(&boot_params->mpc, __va(mpf->physptr), boot_params->mpc.length);
        boot_params->mpf.physptr = __pa(&boot_params->mpc);

        // update MP Floating Pointer (mpf) checksum
        {
            unsigned char checksum = 0;
            int len = 16;
            unsigned char *mp = (unsigned char *) &boot_params->mpf;

            boot_params->mpf.checksum = 0;
            while (len--)
                checksum -= *mp++;
            boot_params->mpf.checksum = checksum;
        }

        printk(KERN_INFO "PISCES: MP tables length %d\n", boot_params->mpc.length); 

        // 3. kick offlined cpu
        kick_offline_cpu();

}
