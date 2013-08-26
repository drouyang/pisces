/* Self-contained header file for
 * shared data structures between host instance and guest instance
 * - spinlock
 * - shared_info
 *   - boot_params
 *   - console buffer
 *
 * Jiannan Ouyang (ouyang@cs.pitt.edu)
 */
#ifndef _PISCES_H_
#define _PISCES_H_

#include <linux/types.h>

#include "pgtables.h"


#define PISCES_MAGIC 0xFE110



/* Pisces Boot loader memory layout

 * 1. boot parameters // 4KB aligned
 *     ->  Trampoline code sits at the start of this structure 
 * 2. Console ring buffer (64KB) // 4KB aligned
 * 3. Identity mapped page tables // 4KB aligned (5 Pages)
 * 4. MPTable // 4KB aligned 
 * 5. kernel image // 2M aligned
 * 6. initrd // 2M aligned
 *
 */

struct pisces_ident_pgt {
    //pml4e64_t     pml[MAX_PML4E64_ENTRIES];
    //pdpe64_t      pdp_phys[MAX_PDPE64_ENTRIES];
    //pdpe64_t      pdp_virt[MAX_PDPE64_ENTRIES];
    //pde64_2MB_t   pd[MAX_PDE64_ENTRIES]; // 512 * 2M = 1G
    pdpe64_t      pdp[MAX_PDPE64_ENTRIES];
    pde64_2MB_t   pd[MAX_PDE64_ENTRIES]; // 512 * 2M = 1G
};




// cpu map, future work
#define PISCES_CPU_MAX 1


/* Intel MP Floating Pointer Structure */
struct pisces_mpf_intel {
	char signature[4];		/* "_MP_"			*/
	u32 physptr;		/* Configuration table address	*/
	u8 length;		/* Our length (paragraphs)	*/
	u8 specification;	/* Specification version	*/
	u8 checksum;		/* Checksum (makes sum 0)	*/
	u8 feature1;		/* Standard or configuration ?	*/
	u8 feature2;		/* Bit7 set for IMCR|PIC	*/
	u8 feature3;		/* Unused (0)			*/
	u8 feature4;		/* Unused (0)			*/
	u8 feature5;		/* Unused (0)			*/
} __attribute__((packed));

struct pisces_mpc_table {
	char signature[4];
	u16 length;		/* Size of table */
	char spec;			/* 0x01 */
	char checksum;
	char oem[8];
	char productid[12];
	u32 oemptr;		/* 0 if not present */
	u16 oemsize;		/* 0 if not present */
	u16 oemcount;
	u32 lapic;		/* APIC address */
	u32 reserved;
} __attribute__((packed));

#define	PISCES_MP_PROCESSOR	0x0
#define	PISCES_MP_BUS		0x1
#define	PISCES_MP_IOAPIC	0x2
#define	PISCES_MP_INTSRC	0x3
#define	PISCES_MP_LINTSRC	0x4

struct pisces_mpc_processor
{               
  u8 type;
  u8 apicid; /* Local APIC number */
  u8 apicver;  /* Its versions */
  u8 cpuflag;
#define PISCES_CPU_ENABLED  0x1 /* Processor is available */
#define PISCES_CPU_BSP      0x2 /* Processor is the BP */
  u32 cpufeature;
  u32 featureflag; /* CPUID feature value */
  u32 reserved[2];
} __attribute__((packed));



/* All addresses in this structure are physical addresses */
struct pisces_boot_params {


    // Embedded asm to load esi and jump to kernel
    u8 launch_code[64]; 

    u8 init_dbg_buf[16];

    u64 magic;
    // cpu map

    u64 boot_params_size;

    u64 cpu_khz;
    
    // coordinator domain cpu apic id
    u64 domain_xcall_master_apicid;

    // domain cross call vector id
    u64 domain_xcall_vector;
  
    // MP table
    struct pisces_mpf_intel mpf; // Intel multiprocessor floating pointer
    struct pisces_mpc_table mpc; // MP config table
    char mpc_entries[1024];  // space for a number of mpc entries
    
    // cmd_line
    char cmd_line[1024];

    // kernel
    u64 kernel_addr;
    u64 kernel_size;
    
    // initrd
    u64 initrd_addr;
    u64 initrd_size;
    

    // The address of the ring buffer used for the early console
    u64 console_ring_addr;
    u64 console_ring_size;

    // 1G ident mapping for guest kernel
    u64 level3_ident_pgt;

    u64 base_mem_paddr;
    u64 base_mem_size;


} __attribute__((packed));




#endif 
