#ifndef _PISCES_LOADER_H_
#define _PISCES_LOADER_H_

#include "pgtables_64.h"
#include "pisces.h"

typedef struct bootstrap_pgt {
  pgd64_t level4_pgt[NUM_PGD_ENTRIES];
  pud64_t level3_ident_pgt[NUM_PUD_ENTRIES];
  pmd64_t level2_ident_pgt[NUM_PMD_ENTRIES]; // 512 * 2M = 1G
  pte64_t level1_ident_pgt[NUM_PMD_ENTRIES]; // 512 * 4K = 2M
  // There is no level 1 pte
} bootstrap_pgt_t;

void pgtable_setup_ident(unsigned long mem_base, unsigned long mem_len);
long load_image(char *path, unsigned long mem_base);
struct boot_params_t *setup_memory_layout(struct pisces_mmap_t *mmap);
int kick_offline_cpu(void);

void start_instance(void);
#endif
