#ifndef __PISCES_XPMEM_H__
#define __PISCES_XPMEM_H__


#ifdef USING_XPMEM

#include "pisces_xbuf.h"
#include "pisces_lcall.h"

#include <xpmem_iface.h>


struct pisces_xpmem {
    /* xbuf for IPI-based communication */
    struct pisces_xbuf_desc      * xbuf_desc;

    /* XPMEM kernel interface */
    int                            connected;
    xpmem_link_t                   link;
    struct xpmem_partition_state * part;
};


int
pisces_xpmem_init(struct pisces_enclave * enclave);

int
pisces_xpmem_cmd_lcall(struct pisces_enclave   * enclave, 
                       struct pisces_xbuf_desc * xbuf_desc, 
		       struct pisces_lcall     * lcall);

#endif /* USING_XPMEM */


#endif /* __PISCES_XPMEM_H__ */
