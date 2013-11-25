#ifndef __PISCES_PORTALS_H__
#define __PISCES_PORTALS_H__

#include "pisces_cmds.h"
#include "util-hashtable.h"

struct portals_ppe_cmd {
    union {
        struct pisces_cmd cmd;
        struct pisces_resp resp;
    } __attribute__((packed));
    u8 data[0];
} __attribute__((packed));

struct xpmem_pfn {
    u64 pfn;
} __attribute__((packed));

struct pisces_xpmem_version_cmd {
    union {
        struct pisces_cmd cmd;
        struct pisces_resp resp;
    } __attribute__((packed));
    struct pisces_xpmem_version pisces_version;
} __attribute__((packed));

struct pisces_xpmem_make_cmd {
    union {
        struct pisces_cmd cmd;
        struct pisces_resp resp;
    } __attribute__((packed));
    struct pisces_xpmem_make pisces_make;
} __attribute__((packed));

struct pisces_xpmem_remove_cmd {
    union {
        struct pisces_cmd cmd;
        struct pisces_resp resp;
    } __attribute__((packed));
    struct pisces_xpmem_remove pisces_remove;
} __attribute__((packed));

struct pisces_xpmem_get_cmd {
    union {
        struct pisces_cmd cmd;
        struct pisces_resp resp;
    } __attribute__((packed));
    struct pisces_xpmem_get pisces_get;
} __attribute__((packed));

struct pisces_xpmem_release_cmd {
    union {
        struct pisces_cmd cmd;
        struct pisces_resp resp;
    } __attribute__((packed));
    struct pisces_xpmem_release pisces_release;
} __attribute__((packed));

struct pisces_xpmem_attach_cmd {
    union {
        struct pisces_cmd cmd;
        struct pisces_resp resp;
    } __attribute__((packed));
    struct pisces_xpmem_attach pisces_attach;
    u64 num_pfns;
    u8 pfns[0];
} __attribute__((packed));

struct pisces_xpmem_detach_cmd {
    union {
        struct pisces_cmd cmd;
        struct pisces_resp resp;
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
    struct pisces_resp * xpmem_resp;
    u8 xpmem_received:1;
    u8 unused:7;
} __attribute__((packed));


int pisces_portals_init(struct pisces_enclave * enclave);
int pisces_portals_connect(struct pisces_enclave * enclave);

/* Kernel level xpmem functions */
int pisces_portals_xpmem_version(struct pisces_enclave * enclave, struct pisces_cmd * cmd);
int pisces_portals_xpmem_make(struct pisces_enclave * enclave, struct pisces_cmd * cmd);
int pisces_portals_xpmem_remove(struct pisces_enclave * enclave, struct pisces_cmd * cmd);
int pisces_portals_xpmem_get(struct pisces_enclave * enclave, struct pisces_cmd * cmd);
int pisces_portals_xpmem_release(struct pisces_enclave * enclave, struct pisces_cmd * cmd);
int pisces_portals_xpmem_attach(struct pisces_enclave * enclave, struct pisces_cmd * cmd);
int pisces_portals_xpmem_detach(struct pisces_enclave * enclave, struct pisces_cmd * cmd);

#endif /* __PISCES_PORTALS_H__ */
