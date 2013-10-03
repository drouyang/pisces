/*
 * Pisces Booting Protocol
 * This file is shared with enclave OS
 */
#ifndef _PISCES_BOOT_PARAMS_H_
#define _PISCES_BOOT_PARAMS_H_

#include <linux/types.h>

#include "pgtables.h"


#define PISCES_MAGIC 0x000FE110

struct pisces_enclave;

/* Pisces Boot loader memory layout

 * 1. boot parameters // 4KB aligned
 *     ->  Trampoline code sits at the start of this structure 
 * 2. Console ring buffer (64KB) // 4KB aligned
 * 3. CMD+CTRL ring buffer // (4KB)
 * 4. Identity mapped page tables // 4KB aligned (5 Pages)
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




/* All addresses in this structure are physical addresses */
struct pisces_boot_params {

    // Embedded asm to load esi and jump to kernel
    u8 launch_code[64]; 

    u8 init_dbg_buf[16];

    u64 magic;

    u64 boot_params_size;

    u64 cpu_id;
    u64 cpu_khz;
    
    // coordinator domain cpu apic id
    u64 domain_xcall_master_apicid;

    // domain cross call vector id
    u64 domain_xcall_vector;

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

    // Address and size of a command/control channel
    u64 control_buf_addr;
    u64 control_buf_size;

    // 1G ident mapping for guest kernel
    u64 level3_ident_pgt;

    u64 base_mem_paddr;
    u64 base_mem_size;


} __attribute__((packed));















#endif 
