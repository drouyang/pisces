#ifndef _GEMINI_H_
#define _GEMINI_H_

#include<asm-x86_64/processor.h>
#define GEMINI_MAGIC 0xABCD

typedef unsigned char gemini_spinlock_t;

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
