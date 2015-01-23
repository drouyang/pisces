#ifndef __PISCES_DATA_H__
#define __PISCES_DATA_H__

#include <linux/types.h>
#include <linux/spinlock.h>
#include <linux/sched.h>


struct pisces_xbuf;
struct pisces_enclave;

struct pisces_xbuf_desc {
    struct pisces_xbuf * xbuf;
    wait_queue_head_t xbuf_waitq;
    spinlock_t xbuf_lock;
    struct pisces_enclave * enclave;
    int irq;

    void (*recv_handler)(struct pisces_enclave * enclave, struct pisces_xbuf_desc * desc);

};


struct pisces_xbuf_desc * pisces_xbuf_server_init(struct pisces_enclave * enclave, 
						  uintptr_t xbuf_va, u32 total_bytes, 
						  void (*recv_handler)(struct pisces_enclave * enclave, 
								       struct pisces_xbuf_desc * desc), 
						  u32 target_cpu);
struct pisces_xbuf_desc * pisces_xbuf_client_init(struct pisces_enclave * enclave, 
						  uintptr_t xbuf_va, 
						  u32 ipi_vector, u32 target_cpu);

int
pisces_xbuf_server_deinit(struct pisces_xbuf_desc * desc);

void
pisces_xbuf_client_deinit(struct pisces_xbuf_desc * desc);

int pisces_xbuf_sync_send(struct pisces_xbuf_desc * desc, u8 * data, u32 data_len, 
		     u8 **resp_data, u32 * resp_len);
int pisces_xbuf_send(struct pisces_xbuf_desc * desc, u8 * data, u32 data_len);

int pisces_xbuf_complete(struct pisces_xbuf_desc * desc, u8 * data, u32 data_len);

int pisces_xbuf_pending(struct pisces_xbuf_desc * desc);
int pisces_xbuf_recv(struct pisces_xbuf_desc * desc, u8 ** data, u32 * data_len);

#endif
