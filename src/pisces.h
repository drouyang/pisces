#ifndef _PISCES_H_
#define _PISCES_H_

/*
 * Name of the device file
 */
#define DEVICE_NAME "pisces"
#define PISCES_PROC_DIR "pisces"

#define MAX_ENCLAVES 128

/* Pisces global cmds */
#define PISCES_LOAD_IMAGE               1001
#define PISCES_ASSIGN_DEVICE            1002
#define PISCES_FREE_ENCLAVE             1003

/* Pisces enclave cmds */
#define PISCES_ENCLAVE_LAUNCH           2000
#define PISCES_ENCLAVE_SHUTDOWN         2001
#define PISCES_ENCLAVE_CONS_CONNECT     2004
#define PISCES_ENCLAVE_CTRL_CONNECT     2005
#define PISCES_ENCLAVE_LCALL_CONNECT    2006
#define PISCES_ENCLAVE_PORTALS_CONNECT  2007
#define PISCES_ENCLAVE_XPMEM_CONNECT    2008


struct enclave_boot_env {
    unsigned long long base_addr;
    unsigned long long pages;
    unsigned int cpu_id;
} __attribute__((packed));


struct pisces_image {
    char kern_path[1024];
    char initrd_path[1024];
    char cmd_line[1024];
} __attribute__((packed));


int device_init(void);
void device_exit(void);
#endif
