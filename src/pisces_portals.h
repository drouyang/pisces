#ifndef __PISCES_PORTALS__
#define __PISCES_PORTALS__

struct portals_ppe_cmd {
    union {
        struct pisces_cmd cmd;
        struct pisces_resp resp;
    } __attribute__ ((packed));
    u8 data[0];
} __attribute__((packed));

struct pisces_portals {
    int connected;
    spinlock_t lock;
} __attribute__((packed));


int pisces_portals_init(struct pisces_enclave * enclave);
int pisces_portals_connect(struct pisces_enclave * enclave);

#endif
