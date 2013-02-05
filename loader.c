#include"inc/loader.h"

#include <linux/fs.h>
#include <linux/buffer_head.h>
#include <asm/segment.h>
#include <asm/uaccess.h>

#define ORDER 2

extern char *path;
bootstrap_pgt_t *bootstrap_pgt = NULL;

void map_offline_memory(unsigned long mem_base, unsigned long mem_len) {

};

void bootstrap_pgtable_init(unsigned long mem_base, unsigned long mem_len)
{
    int i;

    bootstrap_pgt = (bootstrap_pgt_t *) __get_free_pages (GFP_KERNEL, ORDER);

    memset(bootstrap_pgt, 0, sizeof(bootstrap_pgt_t));

    // setup 1G identity mapping
    bootstrap_pgt->level4_pgt[PGD_INDEX(mem_base)].base_addr = PAGE_TO_BASE_ADDR(__pa(bootstrap_pgt->level3_ident_pgt));
    bootstrap_pgt->level4_pgt[PGD_INDEX(mem_base)].present = 1; 
    bootstrap_pgt->level4_pgt[PGD_INDEX(mem_base)].writable = 1; 
    bootstrap_pgt->level4_pgt[PGD_INDEX(mem_base)].accessed = 1; 

    bootstrap_pgt->level3_ident_pgt[PUD_INDEX(mem_base)].base_addr =  PAGE_TO_BASE_ADDR(__pa(bootstrap_pgt->level2_ident_pgt));
    bootstrap_pgt->level3_ident_pgt[PUD_INDEX(mem_base)].present =  1;
    bootstrap_pgt->level3_ident_pgt[PUD_INDEX(mem_base)].writable =  1;
    bootstrap_pgt->level3_ident_pgt[PUD_INDEX(mem_base)].accessed =  1;

    for (i = 0; i < NUM_PMD_ENTRIES; i++) {
        bootstrap_pgt->level2_ident_pgt[i].base_addr = PAGE_TO_BASE_ADDR(__pa(mem_base + (i<<PAGE_POWER_2MB)));
        bootstrap_pgt->level2_ident_pgt[i].present = 1;
        bootstrap_pgt->level2_ident_pgt[i].writable = 1;
        bootstrap_pgt->level2_ident_pgt[i].accessed = 1;
        bootstrap_pgt->level2_ident_pgt[i].large_page = 1;
    }

    return;
}

void load_image(char *path, unsigned long mem_base)
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
        return ;
    }

    // read
    pos = 0;
    vfs_read(filp, __va(mem_base), 512 * 1024 * 1024, &pos);
    printk(KERN_INFO "GEMINI: read %ld bytes (%lu KB) from file\n", (long) pos, (long) pos / 1024);

    //printk(KERN_INFO "GEMINI: %s", (char *)__va(mem_base));
    
    // close 
    set_fs(oldfs);
    filp_close(filp, NULL);
    return;
}

void loader_exit(void) {
    free_pages((unsigned long)bootstrap_pgt, ORDER);
}
