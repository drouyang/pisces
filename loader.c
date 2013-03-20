#include"inc/loader.h"
#include"inc/pgtables_64.h"
#include"inc/gemini.h"

#include <linux/fs.h>
#include <linux/buffer_head.h>
#include <asm/segment.h>
#include <asm/uaccess.h>

#define ORDER 2
#define PAGE_SHIFT_2M 21

bootstrap_pgt_t *bootstrap_pgt = NULL;

void map_offline_memory(unsigned long mem_base, unsigned long mem_len) {

};

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
        printk("GEMINI: level4_pgt va: 0x%lx, pa: 0x%lx\n", 
                (unsigned long) bootstrap_pgt->level4_pgt,
                __pa((unsigned long) bootstrap_pgt->level4_pgt));
        printk("GEMINI: level3_pgt va: 0x%lx, pa: 0x%lx\n", 
                (unsigned long) bootstrap_pgt->level3_ident_pgt,
                __pa((unsigned long) bootstrap_pgt->level3_ident_pgt));
        printk("GEMINI: level2_pgt va: 0x%lx, pa: 0x%lx\n", 
                (unsigned long) bootstrap_pgt->level2_ident_pgt,
                __pa((unsigned long) bootstrap_pgt->level2_ident_pgt));
        printk("GEMINI: level1_pgt va: 0x%lx, pa: 0x%lx\n", 
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
        //printk("GEMINI: level4[%llu].base_addr = %llx\n", PGD_INDEX(mem_base), tmp);

        tmp = PAGE_TO_BASE_ADDR(__pa(bootstrap_pgt->level2_ident_pgt));
        bootstrap_pgt->level3_ident_pgt[PUD_INDEX(mem_base)].base_addr =  tmp;
        bootstrap_pgt->level3_ident_pgt[PUD_INDEX(mem_base)].present =  1;
        bootstrap_pgt->level3_ident_pgt[PUD_INDEX(mem_base)].writable =  1;
        bootstrap_pgt->level3_ident_pgt[PUD_INDEX(mem_base)].accessed =  1;
        //printk("GEMINI: level3[%llu].base_addr = %llx\n", PUD_INDEX(mem_base), tmp);

        tmp = PAGE_TO_BASE_ADDR(__pa(bootstrap_pgt->level1_ident_pgt));
        bootstrap_pgt->level2_ident_pgt[PMD_INDEX(mem_base)].base_addr =  tmp;
        bootstrap_pgt->level2_ident_pgt[PMD_INDEX(mem_base)].present =  1;
        bootstrap_pgt->level2_ident_pgt[PMD_INDEX(mem_base)].writable =  1;
        bootstrap_pgt->level2_ident_pgt[PMD_INDEX(mem_base)].accessed =  1;
        //printk("GEMINI: level2[%llu].base_addr = %llx\n", PMD_INDEX(mem_base), tmp);

        for (i = 0; i < NUM_PTE_ENTRIES; i++) {
            bootstrap_pgt->level1_ident_pgt[i].base_addr = PAGE_TO_BASE_ADDR(mem_base + (i<<PAGE_POWER));
            bootstrap_pgt->level1_ident_pgt[i].present = 1;
            bootstrap_pgt->level1_ident_pgt[i].writable = 1;
            bootstrap_pgt->level1_ident_pgt[i].accessed = 1;
        }
        //printk("GEMINI: level1[0].base_addr = %llx\n", (u64) bootstrap_pgt->level1_ident_pgt[0].base_addr);
        //printk("GEMINI: level1[1].base_addr = %llx\n", (u64) bootstrap_pgt->level1_ident_pgt[1].base_addr);
        //printk("GEMINI: cr3 va: 0x%lx\n", (unsigned long) pgd_base);
        //printk("GEMINI: cr3[0].base_addr: 0x%lx\n", (unsigned long) pgd_base[0].base_addr);
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
    //printk(KERN_INFO "GEMINI: read %ld bytes (%lu KB) from file\n", (long) pos, (long) pos / 1024);

    //printk(KERN_INFO "GEMINI: %s", (char *)__va(addr));

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
struct boot_params_t *setup_memory_layout(struct gemini_mmap_t *mmap) 
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

    printk(KERN_INFO "GEMINI: kernel base 0x%lx size %ld\n", base, size);

    
    /* 2M memory gap between kernel and initrd*/

    // 2. initrd 
    base = mem_base + (512<<PAGE_SHIFT); //roundup
    size = load_image(initrd_path, base);
    local_boot_params.initrd_addr = base;
    local_boot_params.initrd_size = size;

    printk(KERN_INFO "GEMINI: initrd base 0x%lx size %ld\n", base, size);

    // 3. shared_info load to next page
    base += (((size>>PAGE_SHIFT)+1)<<PAGE_SHIFT); //4K roundup
    shared_info = (struct shared_info_t *)__va(base);
    size = sizeof(struct shared_info_t);
    local_boot_params.shared_info_addr = base;
    local_boot_params.shared_info_size = offset;

    printk(KERN_INFO "GEMINI: shared_info base 0x%lx size %ld\n", base, size);

    // copy boot params to offlined memory
    memset((void *)shared_info, 0, sizeof(struct shared_info_t));
    shared_info->magic = GEMINI_MAGIC;
    boot_params = &shared_info->boot_params;
    memcpy(boot_params, &local_boot_params, sizeof(struct boot_params_t));

    memcpy(&boot_params->mmap, mmap, sizeof(struct gemini_mmap_t));

    printk(KERN_INFO "GEMINI: mmap[0] base 0x%llx size 0x%llx\n", boot_params->mmap.map[0].addr, boot_params->mmap.map[0].size);

    return boot_params;
}
