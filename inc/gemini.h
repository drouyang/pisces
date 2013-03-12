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

#define GEMINI_MAGIC 0xABCD

typedef unsigned char gemini_spinlock_t;

/* REP NOP (PAUSE) is a good thing to insert into busy-wait loops. */
static inline void rep_nop(void)
{
	__asm__ __volatile__("rep;nop": : :"memory");
}

#define cpu_relax()   rep_nop()

#define xchg(ptr,v) ((__typeof__(*(ptr)))__xchg((unsigned long)(v),(ptr),sizeof(*(ptr))))

/*
 * Note: no "lock" prefix even on SMP: xchg always implies lock anyway
 * Note 2: xchg has side effect, so that attribute volatile is necessary,
 *	  but generally the primitive is invalid, *ptr is output argument. --ANK
 */
static inline unsigned long __xchg(unsigned long x, volatile void * ptr, int size)
{
	switch (size) {
		case 1:
			__asm__ __volatile__("xchgb %b0,%1"
				:"=q" (x)
				:"m" (*__xg(ptr)), "0" (x)
				:"memory");
			break;
		case 2:
			__asm__ __volatile__("xchgw %w0,%1"
				:"=r" (x)
				:"m" (*__xg(ptr)), "0" (x)
				:"memory");
			break;
		case 4:
			__asm__ __volatile__("xchgl %k0,%1"
				:"=r" (x)
				:"m" (*__xg(ptr)), "0" (x)
				:"memory");
			break;
		case 8:
			__asm__ __volatile__("xchgq %0,%1"
				:"=r" (x)
				:"m" (*__xg(ptr)), "0" (x)
				:"memory");
			break;
	}
	return x;
}

static inline void gemini_spin_init(gemini_spinlock_t *lock)
{
  *lock = 0;
}
static inline void gemini_spin_lock(gemini_spinlock_t *lock)
{
  while (1) {
    if(xchg(lock, 1)) return;
    while(*lock) cpu_relax();
  }
}
static inline void gemini_spin_unlock(gemini_spinlock_t *lock)
{
  barrier();
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
  u32 type;
} __attribute__((packed));

struct gemini_mmap_t {
  int nr_map;
  struct gemini_mmap_entry_t map[GEMINI_MEMMAP_MAX];
} __attribute__((packed));

// cpu map, future work
#define GEMINI_CPU_MAX 1


struct boot_params_t {
  // mem map
  struct gemini_mmap_t gemini_mmap;

  // initrd
  unsigned long start_pa, size;
} __attribute__((packed));

struct shared_info_t {
  u64 magic;
  struct boot_params_t boot_params;
  struct gemini_cons_t console;
} __attribute__((packed));
#endif 
