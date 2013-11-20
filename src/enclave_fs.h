#ifndef __ENCLAVE_FS__
#define __ENCLAVE_FS__


struct hashtable;
struct pisces_enclave;
struct pisces_cmd_buf;


/* LCALL Structs */
struct vfs_buf_desc {
    u32 size;
    u64 phys_addr;
} __attribute__((packed));


struct vfs_read_cmd {
    union {
        struct pisces_cmd cmd;
        struct pisces_resp resp;
    } __attribute__((packed));
    u64 file_handle;
    u64 offset;
    u64 length;
    u32 num_descs;
    struct vfs_buf_desc descs[0];
} __attribute__((packed));

struct vfs_write_cmd {
    union {
        struct pisces_cmd cmd;
        struct pisces_resp resp;
    } __attribute__((packed));
    u64 file_handle;
    u64 offset;
    u64 length;
    u32 num_descs;
    struct vfs_buf_desc descs[0];
} __attribute__((packed));


struct vfs_open_cmd {
    union {
        struct pisces_cmd cmd;
        struct pisces_resp resp;
    } __attribute__((packed));
    u32 mode;
    u8 path[0];
} __attribute__((packed));

struct vfs_close_cmd {
    union {
        struct pisces_cmd cmd;
        struct pisces_resp resp;
    } __attribute__((packed));
    u64 file_handle;
} __attribute__((packed));



struct vfs_size_cmd {
    union {
        struct pisces_cmd cmd;
        struct pisces_resp resp;
    } __attribute__((packed));
    u64 file_handle;
} __attribute__((packed));


struct enclave_fs {
    u32 num_files;
    struct hashtable * open_files;
};


int init_enclave_fs(struct pisces_enclave * enclave);

int enclave_vfs_read_lcall(struct pisces_enclave * enclave, struct pisces_cmd * cmd);
int enclave_vfs_write_lcall(struct pisces_enclave * enclave, struct pisces_cmd * cmd);
int enclave_vfs_open_lcall(struct pisces_enclave * enclave, struct pisces_cmd * cmd);
int enclave_vfs_close_lcall(struct pisces_enclave * enclave, struct pisces_cmd * cmd);
int enclave_vfs_size_lcall(struct pisces_enclave * enclave, struct pisces_cmd * cmd);

#endif
