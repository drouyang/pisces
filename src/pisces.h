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


#define PISCES_MAGIC 0x5a
#define PISCES_MAGIC_MASK 0xff



/* Pisces Boot loader memory layout
 * 1. boot parameters // 4KB aligned
 *     ->  Memory map is appended to this structure
 * 3. Console ring buffer (64KB) // 4KB aligned
 * 4. kernel image // 2M aligned
 * 5. initrd // 2M aligned

 *
 */


// boot memory map
struct pisces_mmap_entry {
  u64 addr;
  u64 size;
} __attribute__((packed));


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

#define	PISCES_MP_PROCESSOR	0
#define	PISCES_MP_BUS		1
#define	PISCES_MP_IOAPIC	2
#define	PISCES_MP_INTSRC	3
#define	PISCES_MP_LINTSRC	4

struct pisces_mpc_processor
{               
  u8 type;
  u8 apicid; /* Local APIC number */
  u8 apicver;  /* Its versions */
  u8 cpuflag;
#define PISCES_CPU_ENABLED  1 /* Processor is available */
#define PISCES_CPU_BSP      2 /* Processor is the BP */
  u32 cpufeature;
  u32 featureflag; /* CPUID feature value */
  u32 reserved[2];
} __attribute__((packed));


struct pisces_boot_params {
    u64 magic;
    // cpu map
    
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


    // This is the address of an array of mmap entries.
    u64 num_mmap_entries;
    struct pisces_mmap_entry mmap[0];

} __attribute__((packed));




#endif 
