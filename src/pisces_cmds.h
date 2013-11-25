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
            u8 active         : 1;   // Set when a command has been activated (initiated)
            u8 staging        : 1;   // Set when partial data is sent
            u8 completed      : 1;   // Set by server OS when command has been handled
            u32 rsvd          : 28;
        } __attribute__((packed));
    } __attribute__((packed));
    
    u32 host_apic;
    u32 host_vector;
    u32 enclave_cpu;
    u32 enclave_vector;
    u32 staging_len;

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
#define ENCLAVE_IOCTL_CREATE_VM 120
#define ENCLAVE_IOCTL_LAUNCH_VM 121


#define ENCLAVE_IOCTL_XPMEM_VERSION 130
#define ENCLAVE_IOCTL_XPMEM_MAKE    131
#define ENCLAVE_IOCTL_XPMEM_REMOVE  132
#define ENCLAVE_IOCTL_XPMEM_GET     133
#define ENCLAVE_IOCTL_XPMEM_RELEASE 134
#define ENCLAVE_IOCTL_XPMEM_ATTACH  135
#define ENCLAVE_IOCTL_XPMEM_DETACH  136

struct memory_range {
    u64 base_addr;
    u64 pages;
} __attribute__((packed));

struct vm_path {
    uint8_t file_name[256];
    uint8_t vm_name[128];
} __attribute__((packed));




/* Kernel Space command Structures */
#ifdef __KERNEL__

struct cmd_cpu_add {
    struct pisces_cmd hdr;
    u64 phys_cpu_id;
    u64 apic_id;
} __attribute__((packed));


struct cmd_mem_add {
    struct pisces_cmd hdr;
    u64 phys_addr;
    u64 size;
} __attribute__((packed));


struct cmd_create_vm {
    struct pisces_cmd hdr;
    struct vm_path path;
} __attribute__((packed));


struct cmd_launch_vm {
    struct pisces_cmd hdr;
    u32 vm_id;
} __attribute__((packed));


#endif

/* ** */


/* 
 * Enclave -> Linux Command Structures
 */

/* Command types */
struct pisces_xpmem_version {
    int64_t version;        /* Input + output parameter */
} __attribute__((packed));

struct pisces_xpmem_make {
    uint64_t vaddr;
    uint64_t size;
    int32_t permit_type;
    uint64_t permit_value;
    int32_t pid;
    int64_t segid;          /* Input + output parameter */
} __attribute__((packed));

struct pisces_xpmem_remove {
    int64_t segid;
    int32_t result;         /* Output parameter */
} __attribute__((packed));

struct pisces_xpmem_get {
    int64_t segid;
    int32_t flags;
    int32_t permit_type;
    uint64_t permit_value;
    int64_t apid;           /* Output parameter */
} __attribute__((packed));

struct pisces_xpmem_release {
    int64_t apid;
    int32_t result;         /* Output parameter */
} __attribute__((packed));

struct pisces_xpmem_attach {
    int64_t apid;
    uint64_t offset;
    uint64_t size;
    int32_t map_fd;         /* Output parameter */
} __attribute__((packed));

struct pisces_xpmem_detach {
    uint64_t vaddr;
    int32_t result;         /* Output parameter */
} __attribute__((packed));



#endif
