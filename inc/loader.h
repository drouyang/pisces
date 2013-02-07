#ifndef _LOADER_H_
#define _LOADER_H_

#include "pgtables_64.h"

typedef struct bootstrap_pgt {
  pgd_2MB_t level4_pgt[NUM_PGD_ENTRIES];
  pud_2MB_t level3_ident_pgt[NUM_PUD_ENTRIES];
  pmd_2MB_t level2_ident_pgt[NUM_PMD_ENTRIES]; // 512 * 2M = 1G
  pte_2MB_t level2_ident_pgt[NUM_PMD_ENTRIES]; // 512 * 2M = 1G
  // There is no level 1 pte
} bootstrap_pgt_t;

void pgtable_setup_ident(unsigned long mem_base, unsigned long mem_len);
long load_image(char *path, unsigned long mem_base);
#endif
