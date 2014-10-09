#ifndef _PISCES_H_
#define _PISCES_H_

/*
 * Name of the device file
 */
#define DEVICE_NAME     "pisces"
#define PISCES_PROC_DIR "pisces"

#define MAX_ENCLAVES 128

/* Pisces global cmds */
#define PISCES_LOAD_IMAGE               1001
#define PISCES_FREE_ENCLAVE             1003
#define PISCES_RELOAD_IMAGE             1004

/* Pisces enclave cmds */
#define PISCES_ENCLAVE_LAUNCH           2000
#define PISCES_ENCLAVE_CONS_CONNECT     2004
#define PISCES_ENCLAVE_CTRL_CONNECT     2005


struct enclave_boot_env {
    unsigned long long base_addr;
    unsigned long long block_size;
    unsigned int       num_blocks;
    unsigned int       cpu_id;
} __attribute__((packed));


struct pisces_image {
    unsigned int kern_fd;
    unsigned int init_fd;
    char         cmd_line[1024];
    int          enclave_id;
} __attribute__((packed));

#endif
