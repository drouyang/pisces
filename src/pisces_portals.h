#ifndef __PISCES_PORTALS_H__
#define __PISCES_PORTALS_H__

#include "../linux_usr/pisces_types.h"

#ifdef __KERNEL__
#include "util-hashtable.h"
#include "pisces_lcall.h"
#include "pagemap.h"
#endif

/* User-level ioctls */
#define ENCLAVE_IOCTL_XPMEM_VERSION     130
#define ENCLAVE_IOCTL_XPMEM_MAKE        131
#define ENCLAVE_IOCTL_XPMEM_REMOVE      132
#define ENCLAVE_IOCTL_XPMEM_GET         133
#define ENCLAVE_IOCTL_XPMEM_RELEASE     134
#define ENCLAVE_IOCTL_XPMEM_ATTACH      135
#define ENCLAVE_IOCTL_XPMEM_DETACH      136

struct pisces_enclave;

/* User-level command structures */
struct pisces_xpmem_version {
    int64_t version;            /* Output parameter */
} __attribute__((packed));

struct pisces_xpmem_make {
    uint64_t vaddr;
    uint64_t size;
    int32_t permit_type;
    uint64_t permit_value;
    int32_t pid;
    int64_t segid;              /* Output parameter */
} __attribute__((packed));

struct pisces_xpmem_remove {
    int64_t segid;
    int32_t result;             /* Output paramater */
} __attribute__((packed));

struct pisces_xpmem_get {
    int64_t segid;
    int32_t flags;
    int32_t permit_type;
    uint64_t permit_value;
    int64_t apid;               /* Output parameter */
} __attribute__((packed));

struct pisces_xpmem_release {
    int64_t apid;
    int32_t result;             /* Output parameter */
} __attribute__((packed));

struct pisces_xpmem_attach {
    int64_t apid;
    uint64_t offset;
    uint64_t size;
    uint64_t vaddr;             /* Output parameter */
} __attribute__((packed));

struct pisces_xpmem_detach {
    uint64_t vaddr;
    int32_t result;             /* Output parameter */
} __attribute__((packed));


int pisces_portals_init(struct pisces_enclave * enclave);
int pisces_portals_connect(struct pisces_enclave * enclave);


#ifdef __KERNEL__
/* In-kernel command structures */

/* General purpose */
struct pisces_ppe_lcall {
    union {
        struct pisces_lcall lcall;
        struct pisces_lcall_resp lcall_resp;
    } __attribute__((packed));
    u8 data[0];
} __attribute__((packed));


/* Xpmem */
struct pisces_xpmem_version_lcall {
    union {
        struct pisces_lcall lcall;
        struct pisces_lcall_resp lcall_resp;
    } __attribute__((packed));
    struct pisces_xpmem_version pisces_version;
} __attribute__((packed));

struct pisces_xpmem_make_lcall {
    union {
        struct pisces_lcall lcall;
        struct pisces_lcall_resp lcall_resp;
    } __attribute__((packed));
    struct pisces_xpmem_make pisces_make;
    struct enclave_aspace aspace;
} __attribute__((packed));

struct pisces_xpmem_remove_lcall {
    union {
        struct pisces_lcall lcall;
        struct pisces_lcall_resp lcall_resp;
    } __attribute__((packed));
    struct pisces_xpmem_remove pisces_remove;
} __attribute__((packed));
struct pisces_xpmem_get_lcall {
    union {
        struct pisces_lcall lcall;
        struct pisces_lcall_resp lcall_resp;
    } __attribute__((packed));
    struct pisces_xpmem_get pisces_get;
} __attribute__((packed));

struct pisces_xpmem_release_lcall {
    union {
        struct pisces_lcall lcall;
        struct pisces_lcall_resp lcall_resp;
    } __attribute__((packed));
    struct pisces_xpmem_release pisces_release;
} __attribute__((packed));

struct pisces_xpmem_attach_lcall {
    union {
        struct pisces_lcall lcall;
        struct pisces_lcall_resp lcall_resp;
    } __attribute__((packed));
    struct pisces_xpmem_attach pisces_attach;
    u64 num_pfns;
    u8 pfns[0];
} __attribute__((packed));

struct pisces_xpmem_detach_lcall {
    union {
        struct pisces_lcall lcall;
        struct pisces_lcall_resp lcall_resp;
    } __attribute__((packed));
    struct pisces_xpmem_detach pisces_detach;
} __attribute__((packed));



struct pisces_xpmem_info {
    struct pisces_xpmem_version xpmem_version;
};

struct pisces_portals {
    int connected;
    int fd;
    spinlock_t lock;

    struct pisces_xpmem_info xpmem_info;
    struct hashtable * xpmem_map;

    /* For user space handling */
    wait_queue_head_t user_waitq;
    struct pisces_ppe_lcall * ppe_lcall;
    struct pisces_xbuf_desc * xbuf_desc;
} __attribute__((packed));

/* User-level lcall callbacks */
int pisces_portals_ppe_message(struct pisces_enclave * enclave,
        struct pisces_xbuf_desc * xbuf_desc,
        struct pisces_lcall * lcall);


/* Kernel-level lcall callbacks */
int pisces_portals_xpmem_version(struct pisces_enclave * enclave,
        struct pisces_xbuf_desc * xbuf_desc,
        struct pisces_lcall * lcall);

int pisces_portals_xpmem_make(struct pisces_enclave * enclave,
        struct pisces_xbuf_desc * xbuf_desc,
        struct pisces_lcall * lcall);

int pisces_portals_xpmem_remove(struct pisces_enclave * enclave,
        struct pisces_xbuf_desc * xbuf_desc,
        struct pisces_lcall * lcall);

int pisces_portals_xpmem_get(struct pisces_enclave * enclave,
        struct pisces_xbuf_desc * xbuf_desc,
        struct pisces_lcall * lcall);

int pisces_portals_xpmem_release(struct pisces_enclave * enclave,
        struct pisces_xbuf_desc * xbuf_desc,
        struct pisces_lcall * lcall);

int pisces_portals_xpmem_attach(struct pisces_enclave * enclave,
        struct pisces_xbuf_desc * xbuf_desc,
        struct pisces_lcall * lcall);

int pisces_portals_xpmem_detach(struct pisces_enclave * enclave,
        struct pisces_xbuf_desc * xbuf_desc,
        struct pisces_lcall * lcall);


#endif /* __KERNEL__ */

#endif /* __PISCES_PORTALS_H__ */
