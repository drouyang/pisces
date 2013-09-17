#ifndef _PISCES_H_
#define _PISCES_H_

/* 
 * The name of the device file 
 */
#define DEVICE_NAME "pisces"
#define PISCES_PROC_DIR "pisces"

/* Pisces global ioctls */

#define P_IOCTL_MIN    1000
#define P_IOCTL_PING    1000

// Initialized mem_base and mem_size params before prepare secondary
// This will setup physical memory and identity mapping for the secondary cpu
#define P_IOCTL_PREPARE_SECONDARY    1001

#define P_IOCTL_LOAD_IMAGE    1002
#define P_IOCTL_LAUNCH_ENCLAVE    1100

// Boot secondary cpu via INIT, secondary cpu will jump to hooker() in long mode
#define P_IOCTL_START_SECONDARY    1003

#define P_IOCTL_PRINT_IMAGE    1004

#define P_IOCTL_EXIT 1005

#define P_IOCTL_MAX    1005

#define P_IOCTL_ADD_MEM 1006

struct memory_range {
    unsigned long long base_addr;
    unsigned long long pages;
} __attribute__((packed));



struct pisces_image {
    char kern_path[1024];
    char initrd_path[1024];
    char cmd_line[1024];

    unsigned long long memsize_in_bytes;
    unsigned long long num_cpus;
} __attribute__((packed));


int device_init(void);
void device_exit(void);
#endif
