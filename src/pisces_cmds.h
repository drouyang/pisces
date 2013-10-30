#ifndef __PISCES_CMDS_H__
#define __PISCES_CMDS_H__

#ifndef __KERNEL__
#include "../linux_usr/pisces_types.h"
#endif



/* 
 * Generic Pisces Control Channel structures 
 */
struct pisces_cmd {
    u64 cmd;
    u32 data_len;
    u8 data[0];
} __attribute__((packed));


struct pisces_resp {
    u64 status;
    u32 data_len;
    u8 data[0];
} __attribute__((packed));


#ifdef __KERNEL__
struct pisces_cmd_buf {
    union {
	u64 flags;
	struct {
	    u8 ready          : 1;   // Flag set by server OS, after channel is init'd
	    u8 active         : 1;   // Set by client OS when a command is in progress
	    u8 completed      : 1;   // Set by server OS when command has been handled
	    u32 rsvd          : 29;
	} __attribute__((packed));
    } __attribute__((packed));
    
    u32 host_apic;
    u32 host_vector;
    u32 enclave_cpu;
    u32 enclave_vector;

    union {
	struct pisces_cmd cmd;
	struct pisces_resp resp;
    } __attribute__((packed));
    
} __attribute__((packed));
#endif





/* 
 * Linux -> Enclave Command Structures
 */

/* User space ioctl structures */

#define ENCLAVE_IOCTL_ADD_CPU 100
#define ENCLAVE_IOCTL_ADD_MEM 101
#define ENCLAVE_IOCTL_TEST_LCALL 102


struct memory_range {
    u64 base_addr;
    u64 pages;
} __attribute__((packed));


/* Kernel Space command Structures */
#ifdef __KERNEL__

#define ENCLAVE_CMD_ADD_CPU      100
#define ENCLAVE_CMD_ADD_MEM      101
#define ENCLAVE_CMD_TEST_LCALL     102

struct cmd_cpu_add {
    struct pisces_cmd hdr;
    u64 phys_cpu_id;
} __attribute__((packed));


struct cmd_mem_add {
    struct pisces_cmd hdr;
    u64 phys_addr;
    u64 size;
} __attribute__((packed));

#endif

/* ** */


/* 
 * Enclave -> Linux Command Structures
 */



#endif
