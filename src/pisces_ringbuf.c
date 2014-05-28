/* Pisces Ringbuffer 
 * Basic communication channel between enclaves
 * (c) 2013, Jack Lange (jacklange@cs.pitt.edu)
 * (c) 2013, Jiannan Ouyang (ouyang@cs.pitt.edu)
 */


#include <linux/module.h>
#include "pisces_ringbuf.h"

static inline u8 * get_read_ptr(struct pisces_early_ringbuf * ring) {
    return (u8 *)(ring->buf + ring->read_idx);
}


static inline u8 * get_write_ptr(struct pisces_early_ringbuf * ring) {
    return (u8 *)(ring->buf + ring->write_idx);
}


static inline int is_read_loop(struct pisces_early_ringbuf * ring, unsigned int len) {
    if ( (ring->read_idx >= ring->write_idx) &&
	 (ring->cur_len  > 0) ) {
	// end is past the end of the buffer
	if ((ring->size - ring->read_idx) < len) {
	    return 1;
	}
    }
    return 0;
}


static inline int is_write_loop(struct pisces_early_ringbuf * ring, unsigned int len) {
    if ( (ring->write_idx >= ring->read_idx) && 
	 (ring->cur_len   < ring->size) ) {

	// end is past the end of the buffer
	if ((ring->size - ring->write_idx) < len) {
	    return 1;
	}
    }
    return 0;
}





int pisces_early_ringbuf_init(struct pisces_early_ringbuf * ring) {
    memset(ring, 0, sizeof(struct pisces_early_ringbuf));

    ring->size = EARLY_RINGBUF_SIZE;
    pisces_lock_init(&(ring->lock));

    return 0;
}


int 
pisces_early_ringbuf_write(struct pisces_early_ringbuf * ring, 
			   u8                          * data, 
			   u64                           write_len)
{

    int ret         = 0;
    int avail_space = 0;

    pisces_spin_lock(&(ring->lock));
    {
	avail_space = ring->size - ring->cur_len;
	
	if (write_len > avail_space) {
	    ret = -1;
	} else {

	    if (is_write_loop(ring, write_len)) {
		int section_len = ring->size - ring->write_idx;
		
		memcpy(get_write_ptr(ring), data, section_len);
		ring->write_idx = 0;
		memcpy(get_write_ptr(ring), data + section_len, write_len - section_len);
		
		ring->write_idx += write_len - section_len;
	    } else {
		memcpy(get_write_ptr(ring), data, write_len);
		
		ring->write_idx += write_len;
	    }
	    
	    ring->cur_len += write_len;

	}
    }
    pisces_spin_unlock(&(ring->lock));

    return ret;

}



int 
pisces_early_ringbuf_read(struct pisces_early_ringbuf * ring, 
			  u8                          * data, 
			  u64                           read_len) 
{
    int ret = 0;

    pisces_spin_lock(&(ring->lock));
    {
	if (read_len > ring->cur_len) {
	    ret = -1;
	} else {
	    
	    if (is_read_loop(ring, read_len)) {
		int section_len = ring->size - ring->read_idx;
		
		memcpy(data,  get_read_ptr(ring), section_len);
		ring->read_idx = 0;
		memcpy(data + section_len, get_read_ptr(ring), read_len - section_len);
		
		ring->read_idx += read_len - section_len;
	    } else {
		memcpy(data, get_read_ptr(ring), read_len);
		
		ring->read_idx += read_len;
	    }
	    
	    ring->cur_len -= read_len;
	}
    }
    pisces_spin_unlock(&(ring->lock));

    return ret;
}


int pisces_early_ringbuf_is_full(struct pisces_early_ringbuf * ring) {

    return ring->cur_len != 0;
}


int pisces_early_ringbuf_is_empty(struct pisces_early_ringbuf * ring) {
    return ring->cur_len == 0;
}
