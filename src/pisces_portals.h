#ifndef __PISCES_PORTALS__
#define __PISCES_PORTALS__

#define MAX_PPE_MSG_LEN 256
struct portals_ppe_cmd {
    struct pisces_cmd cmd;
    char msg[MAX_PPE_MSG_LEN];
} __attribute__((packed));

struct pisces_portals {
    int connected;
    spinlock_t lock;
} __attribute__((packed));


int pisces_portals_init(struct pisces_enclave * enclave);
int pisces_portals_connect(struct pisces_enclave * enclave);

#endif
