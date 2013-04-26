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




// boot memory map
#define PISCES_MEMMAP_MAX 1
struct pisces_mmap_entry_t {
  u64 addr;
  u64 size;
} __attribute__((packed));

struct pisces_mmap_t {
  int nr_map;
  struct pisces_mmap_entry_t map[PISCES_MEMMAP_MAX];
} __attribute__((packed));

// cpu map, future work
#define PISCES_CPU_MAX 1


/* Intel MP Floating Pointer Structure */
struct pisces_mpf_intel {
	char signature[4];		/* "_MP_"			*/
	unsigned int physptr;		/* Configuration table address	*/
	unsigned char length;		/* Our length (paragraphs)	*/
	unsigned char specification;	/* Specification version	*/
	unsigned char checksum;		/* Checksum (makes sum 0)	*/
	unsigned char feature1;		/* Standard or configuration ?	*/
	unsigned char feature2;		/* Bit7 set for IMCR|PIC	*/
	unsigned char feature3;		/* Unused (0)			*/
	unsigned char feature4;		/* Unused (0)			*/
	unsigned char feature5;		/* Unused (0)			*/
};

struct pisces_mpc_table {
	char signature[4];
	unsigned short length;		/* Size of table */
	char spec;			/* 0x01 */
	char checksum;
	char oem[8];
	char productid[12];
	unsigned int oemptr;		/* 0 if not present */
	unsigned short oemsize;		/* 0 if not present */
	unsigned short oemcount;
	unsigned int lapic;		/* APIC address */
	unsigned int reserved;
};

#define	PISCES_MP_PROCESSOR	0
#define	PISCES_MP_BUS		1
#define	PISCES_MP_IOAPIC	2
#define	PISCES_MP_INTSRC	3
#define	PISCES_MP_LINTSRC	4

struct pisces_mpc_processor
{               
  unsigned char type;
  unsigned char apicid; /* Local APIC number */
  unsigned char apicver;  /* Its versions */
  unsigned char cpuflag;
#define PISCES_CPU_ENABLED  1 /* Processor is available */
#define PISCES_CPU_BSP      2 /* Processor is the BP */
  unsigned int cpufeature;
  unsigned int featureflag; /* CPUID feature value */
  unsigned int reserved[2];
};


struct boot_params_t {
  // cpu map

  // coordinator domain cpu apic id
  unsigned long domain_xcall_master_apicid;

  // domain cross call vector id
  unsigned long domain_xcall_vector;
  
  // mem map
  struct pisces_mmap_t mmap;

  // MP table
  struct pisces_mpf_intel mpf; // Intel multiprocessor floating pointer
  struct pisces_mpc_table mpc; // MP config table
  char mpc_entries[1024];  // space for a number of mpc entries

  // cmd_line
  char cmd_line[1024];

  // kernel
  unsigned long kernel_addr;
  unsigned long kernel_size;

  // initrd
  unsigned long initrd_addr;
  unsigned long initrd_size;

  // shared_info
  unsigned long shared_info_addr;
  unsigned long shared_info_size;
} __attribute__((packed));


#include "pisces_cons.h"


struct shared_info {
    u64 magic;
    struct boot_params_t boot_params;
    struct pisces_cons console;
} __attribute__((packed));


#endif 
