#include"inc/loader.h"
#include"inc/pgtables_64.h"

#include <linux/fs.h>
#include <linux/buffer_head.h>
#include <asm/segment.h>
#include <asm/uaccess.h>

#define ORDER 2

extern char *path;
bootstrap_pgt_t *bootstrap_pgt = NULL;

void map_offline_memory(unsigned long mem_base, unsigned long mem_len) {

};

void pgtable_setup_ident(unsigned long mem_base, unsigned long mem_len)
{

    u64 cr3;
    pgd64_t *pgd_base, *pgde;


    cr3 = get_cr3();

    pgd_base = (pgd64_t *) CR3_TO_PGD_VA(cr3);

    pgde = pgd_base + PGD_INDEX(mem_base);

    {
        // setup 1G identity mapping
        int i;
        u64 tmp;

        bootstrap_pgt = (bootstrap_pgt_t *) __get_free_pages (GFP_KERNEL, ORDER);
        memset(bootstrap_pgt, 0, sizeof(bootstrap_pgt_t));
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
        printk("GEMINI: level4[%llu].base_addr = %llx\n", PGD_INDEX(mem_base), tmp);

        tmp = PAGE_TO_BASE_ADDR(__pa(bootstrap_pgt->level2_ident_pgt));
        bootstrap_pgt->level3_ident_pgt[PUD_INDEX(mem_base)].base_addr =  tmp;
        bootstrap_pgt->level3_ident_pgt[PUD_INDEX(mem_base)].present =  1;
        bootstrap_pgt->level3_ident_pgt[PUD_INDEX(mem_base)].writable =  1;
        bootstrap_pgt->level3_ident_pgt[PUD_INDEX(mem_base)].accessed =  1;
        printk("GEMINI: level3[%llu].base_addr = %llx\n", PUD_INDEX(mem_base), tmp);

        tmp = PAGE_TO_BASE_ADDR(__pa(bootstrap_pgt->level1_ident_pgt));
        bootstrap_pgt->level2_ident_pgt[PMD_INDEX(mem_base)].base_addr =  tmp;
        bootstrap_pgt->level2_ident_pgt[PMD_INDEX(mem_base)].present =  1;
        bootstrap_pgt->level2_ident_pgt[PMD_INDEX(mem_base)].writable =  1;
        bootstrap_pgt->level2_ident_pgt[PMD_INDEX(mem_base)].accessed =  1;
        printk("GEMINI: level2[%llu].base_addr = %llx\n", PMD_INDEX(mem_base), tmp);

        for (i = 0; i < NUM_PTE_ENTRIES; i++) {
            bootstrap_pgt->level1_ident_pgt[i].base_addr = PAGE_TO_BASE_ADDR(mem_base + (i<<PAGE_POWER));
            bootstrap_pgt->level1_ident_pgt[i].present = 1;
            bootstrap_pgt->level1_ident_pgt[i].writable = 1;
            bootstrap_pgt->level1_ident_pgt[i].accessed = 1;
        }
        printk("GEMINI: level1[0].base_addr = %llx\n", (u64) bootstrap_pgt->level1_ident_pgt[0].base_addr);
        printk("GEMINI: level1[1].base_addr = %llx\n", (u64) bootstrap_pgt->level1_ident_pgt[1].base_addr);
        //printk("GEMINI: cr3 va: 0x%lx\n", (unsigned long) pgd_base);
        //printk("GEMINI: cr3[0].base_addr: 0x%lx\n", (unsigned long) pgd_base[0].base_addr);
    }

    return;
}

long load_image(char *path, unsigned long mem_base)
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
    vfs_read(filp, __va(mem_base), 512 * 1024 * 1024, &pos);
    //printk(KERN_INFO "GEMINI: read %ld bytes (%lu KB) from file\n", (long) pos, (long) pos / 1024);

    //printk(KERN_INFO "GEMINI: %s", (char *)__va(mem_base));

    // close 
    set_fs(oldfs);
    filp_close(filp, NULL);
    return (long) pos;
}

void loader_exit(void) {
    free_pages((unsigned long)bootstrap_pgt, ORDER);
}
