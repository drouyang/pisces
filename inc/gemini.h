/* Self-contained header file for
 * shared data structures between host instance and guest instance
 * - spinlock
 * - shared_info
 *   - boot_params
 *   - console buffer
 *
 * Jiannan Ouyang (ouyang@cs.pitt.edu)
 */
#ifndef _GEMINI_H_
#define _GEMINI_H_

#define GEMINI_MAGIC 0x5a
#define GEMINI_MAGIC_MASK 0xff
typedef unsigned long long u64;
typedef unsigned int u32;


/* REP NOP (PAUSE) is a good thing to insert into busy-wait loops. */
static inline void gemini_cpu_relax(void)
{
	__asm__ __volatile__("rep;nop": : :"memory");
}

/*
 * Note: no "lock" prefix even on SMP: xchg always implies lock anyway
 * Note 2: xchg has side effect, so that attribute volatile is necessary,
 *	  but generally the primitive is invalid, *ptr is output argument. --ANK
 */
static inline unsigned long gemini_xchg8(volatile void * ptr, unsigned char x )
{
  __asm__ __volatile__("xchgb %0,%1"
                       :"=r" (x)
                       :"m" (*(volatile unsigned char *)ptr), "0" (x)
                       :"memory");
  return x;
}
typedef unsigned char gemini_spinlock_t;

static inline void gemini_spin_init(gemini_spinlock_t *lock)
{
  *lock = 0;
}
static inline void gemini_spin_lock(gemini_spinlock_t *lock)
{
  while (1) {
    if(gemini_xchg8(lock, 1)) return;
    while(*lock) gemini_cpu_relax();
  }
}
static inline void gemini_spin_unlock(gemini_spinlock_t *lock)
{
  __asm__ __volatile__ ("": : :"memory");
  *lock = 0;
}

// in out buffer as a circular queue
// prod is queue head, point to an available slot
// cons is queue tail
// cons == prod-1 => queue is empty
// prod == cons-1 => queue is full
#define GEMINI_CONSOLE_SIZE_OUT 2048
#define GEMINI_CONSOLE_SIZE_IN 1024
struct gemini_cons_t {
  // in buffer 1K
  gemini_spinlock_t lock_in;
  char in[GEMINI_CONSOLE_SIZE_IN];
  u64 in_cons, in_prod;

  // out buffer 2K
  gemini_spinlock_t lock_out;
  char out[GEMINI_CONSOLE_SIZE_OUT];
  u64 out_cons, out_prod;

} __attribute__((packed));

// boot memory map
#define GEMINI_MEMMAP_MAX 1
struct gemini_mmap_entry_t {
  u64 addr;
  u64 size;
} __attribute__((packed));

struct gemini_mmap_t {
  int nr_map;
  struct gemini_mmap_entry_t map[GEMINI_MEMMAP_MAX];
} __attribute__((packed));

// cpu map, future work
#define GEMINI_CPU_MAX 1


struct boot_params_t {
  // mem map
  struct gemini_mmap_t mmap;

  // cmd_line
  char cmd_line[255];

  // shared_info
  unsigned long shared_info_addr;
  unsigned long shared_info_size;

  // kernel
  unsigned long kernel_addr;
  unsigned long kernel_size;

  // initrd
  unsigned long initrd_addr;
  unsigned long initrd_size;
} __attribute__((packed));

struct shared_info_t {
  u64 magic;
  struct boot_params_t boot_params;
  struct gemini_cons_t console;
} __attribute__((packed));
#endif 
