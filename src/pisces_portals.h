#ifndef __PISCES_PORTALS__
#define __PISCES_PORTALS__

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

struct pisces_xpmem_make_cmd {
    union {
        struct pisces_cmd cmd;
        struct pisces_resp resp;
    } __attribute__((packed));
    struct pisces_xpmem_make pisces_make;
} __attribute__((packed));

struct pisces_xpmem_get_cmd {
    union {
        struct pisces_cmd cmd;
        struct pisces_resp resp;
    } __attribute__((packed));
    struct pisces_xpmem_get pisces_get;
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

struct pisces_portals {
    int connected;
    spinlock_t lock;
    struct hashtable * xpmem_map;
} __attribute__((packed));


int pisces_portals_init(struct pisces_enclave * enclave);
int pisces_portals_connect(struct pisces_enclave * enclave);

/* Kernel level xpmem functions */
int pisces_portals_xpmem_attach(struct pisces_enclave * enclave, struct pisces_cmd * cmd);

#endif
