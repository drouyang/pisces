	.text

	.code64
.balign PAGE_SIZE

#define NEXT_PAGE(name) \
	.balign	PAGE_SIZE; \
ENTRY(name)

/* Automate the creation of 1 to 1 mapping pmd entries */
#define PMDS(START, PERM, COUNT)		\
	i = 0 ;					\
	.rept (COUNT) ;				\
	.quad	(START) + (i << 21) + (PERM) ;	\
	i = i + 1 ;				\
	.endr

	/*
	 * This default setting generates an ident mapping at address 0x100000
	 * and a mapping for the kernel that precisely maps virtual address
	 * 0xffffffff80000000 to physical address 0x000000. (always using
	 * 2Mbyte large pages provided by PAE mode)
	 */
NEXT_PAGE(init_level4_pgt)
	.quad	level3_ident_pgt - __START_KERNEL_map + _KERNPG_TABLE
	.fill	257,8,0
	.quad	level3_ident_pgt - __START_KERNEL_map + _KERNPG_TABLE
	.fill	252,8,0
	/* (2^48-(2*1024*1024*1024))/(2^39) = 511 */
	.quad	level3_kernel_pgt - __START_KERNEL_map + _PAGE_TABLE

NEXT_PAGE(level3_ident_pgt)
	.quad	level2_ident_pgt - __START_KERNEL_map + _KERNPG_TABLE
	.fill	511,8,0

NEXT_PAGE(level3_kernel_pgt)
	.fill	510,8,0
	/* (2^48-(2*1024*1024*1024)-((2^39)*511))/(2^30) = 510 */
	.quad	level2_kernel_pgt - __START_KERNEL_map + _KERNPG_TABLE
        .fill   1,8,0

NEXT_PAGE(level2_ident_pgt)
	/* Since I easily can, map the first 1G.
	 * Don't set NX because code runs from these pages.
	 */
	PMDS(0x0000000000000000, __PAGE_KERNEL_LARGE_EXEC, PTRS_PER_PMD)

NEXT_PAGE(level2_kernel_pgt)
	/* 40MB kernel mapping. The kernel code cannot be bigger than that.
	   When you change this change KERNEL_TEXT_SIZE in page.h too. */
	/* (2^48-(2*1024*1024*1024)-((2^39)*511)-((2^30)*510)) = 0 */
	PMDS(0x0000000000000000, __PAGE_KERNEL_LARGE_EXEC|_PAGE_GLOBAL, KERNEL_TEXT_SIZE/PMD_SIZE)
	/* Module mapping starts here */
	.fill	(PTRS_PER_PMD - (KERNEL_TEXT_SIZE/PMD_SIZE)),8,0


NEXT_PAGE(level2_spare_pgt)
	.fill   512,8,0

#undef PMDS
#undef NEXT_PAGE

ENTRY(phys_base)
	/* This must match the first entry in level2_kernel_pgt */
	.quad   0x0000000000000000

/* We need valid kernel segments for data and code in long mode too
 * IRET will check the segment types  kkeil 2000/10/28
 * Also sysret mandates a special GDT layout 
 */
		 		
	.section .data.page_aligned, "aw"
	.align PAGE_SIZE

/* The TLS descriptors are currently at a different place compared to i386.
   Hopefully nobody expects them at a fixed place (Wine?) */
	
ENTRY(cpu_gdt_table)
	.quad	0x0000000000000000	/* NULL descriptor */
	.quad	0x00cf9b000000ffff	/* __KERNEL32_CS */
	.quad	0x00af9b000000ffff	/* __KERNEL_CS */
	.quad	0x00cf93000000ffff	/* __KERNEL_DS */
	.quad	0x00cffb000000ffff	/* __USER32_CS */
	.quad	0x00cff3000000ffff	/* __USER_DS, __USER32_DS  */
	.quad	0x00affb000000ffff	/* __USER_CS */
	.quad	0x0			/* unused */
	.quad	0,0			/* TSS */
	.quad	0,0			/* LDT */
	.quad   0,0,0			/* three TLS descriptors */ 
	.quad	0x0000f40000000000	/* node/CPU stored in limit */
gdt_end:	
	/* asm/segment.h:GDT_ENTRIES must match this */	
	/* This should be a multiple of the cache line size */
	/* GDTs of other CPUs are now dynamically allocated */

	/* zero the remaining page */
	.fill PAGE_SIZE / 8 - GDT_ENTRIES,8,0

	.section .bss, "aw", @nobits
	.align L1_CACHE_BYTES
ENTRY(idt_table)
	.skip 256 * 16

	.section .bss.page_aligned, "aw", @nobits
	.align PAGE_SIZE
ENTRY(empty_zero_page)
	.skip PAGE_SIZE
