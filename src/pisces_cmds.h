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



/* 
 * Linux -> Enclave Command Structures
 */

/* User space ioctl structures */

#define ENCLAVE_CMD_ADD_CPU 100
#define ENCLAVE_CMD_ADD_MEM 101
#define ENCLAVE_CMD_TEST_LCALL 102
#define ENCLAVE_CMD_CREATE_VM 120
#define ENCLAVE_CMD_LAUNCH_VM 121
#define ENCLAVE_CMD_VM_CONS_CONNECT 122
#define ENCLAVE_CMD_VM_CONS_DISCONNECT 123
#define ENCLAVE_CMD_VM_CONS_KEYCODE 124
#define ENCLAVE_CMD_ADD_V3_PCI 125

#define ENCLAVE_CMD_VM_DBG 169


struct memory_range {
    u64 base_addr;
    u64 pages;
} __attribute__((packed));

struct vm_path {
    uint8_t file_name[256];
    uint8_t vm_name[128];
} __attribute__((packed));

struct pisces_pci_dev {
    u8 name[128];
    u32 domain;
    u32 bus;
    u32 dev;
    u32 func;
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


struct cmd_vm_cons_connect {
    struct pisces_cmd hdr;
    u32 vm_id;
} __attribute__((packed));


struct cmd_vm_cons_disconnect {
    struct pisces_cmd hdr;
    u32 vm_id;
} __attribute__((packed));


struct cmd_vm_cons_keycode {
    struct pisces_cmd hdr;
    u32 vm_id;
    u8 scan_code;
} __attribute__((packed));


struct cmd_add_pci_dev {
	struct pisces_cmd hdr;
	struct pisces_pci_dev device;
} __attribute__((packed));


#endif

/* ** */


/* 
 * Enclave -> Linux Command Structures
 */

#endif
