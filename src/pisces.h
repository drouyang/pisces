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

#define PISCES_MAGIC 0x5a
#define PISCES_MAGIC_MASK 0xff
typedef unsigned long long u64;
typedef unsigned int u32;


/* REP NOP (PAUSE) is a good thing to insert into busy-wait loops. */
static inline void pisces_cpu_relax(void)
{
	__asm__ __volatile__("rep;nop": : :"memory");
}

/*
 * Note: no "lock" prefix even on SMP: xchg always implies lock anyway
 * Note 2: xchg has side effect, so that attribute volatile is necessary,
 *	  but generally the primitive is invalid, *ptr is output argument. --ANK
 */
static inline unsigned long pisces_xchg8(volatile void * ptr, unsigned char x )
{
  __asm__ __volatile__("xchgb %0,%1"
                       :"=r" (x)
                       :"m" (*(volatile unsigned char *)ptr), "0" (x)
                       :"memory");
  return x;
}
typedef unsigned char pisces_spinlock_t;

static inline void pisces_spin_init(pisces_spinlock_t *lock)
{
  *lock = 0;
}
static inline void pisces_spin_lock(pisces_spinlock_t *lock)
{
  while (1) {
    if(pisces_xchg8(lock, 1)==0) return;
    while(*lock) pisces_cpu_relax();
  }
}
static inline void pisces_spin_unlock(pisces_spinlock_t *lock)
{
  __asm__ __volatile__ ("": : :"memory");
  *lock = 0;
}

// in out buffer as a circular queue
// prod is queue head, point to an available slot
// cons is queue tail
// cons == prod-1 => queue is empty
// prod == cons-1 => queue is full
#define PISCES_CONSOLE_SIZE_OUT (1024*6)
#define PISCES_CONSOLE_SIZE_IN 1024
struct pisces_cons_t {
  // in buffer 1K
  //pisces_spinlock_t lock_in;
  //char in[PISCES_CONSOLE_SIZE_IN];
  //u64 in_cons, in_prod;

  // out buffer 2K
  pisces_spinlock_t lock_out;
  char out[PISCES_CONSOLE_SIZE_OUT];
  u64 out_cons, out_prod;

} __attribute__((packed));

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

struct shared_info_t {
  u64 magic;
  struct boot_params_t boot_params;
  struct pisces_cons_t console;
} __attribute__((packed));
#endif 
