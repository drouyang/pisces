/* 
 * XPMEM communication channel
 *
 * All we're really doing is forwarding commands between the Linux XPMEM
 * domain and the enclave domain
 *
 * (c) Brian Kocoloski, 2013 (briankoco@cs.pitt.edu)
 */

#include <linux/types.h>
#include <linux/anon_inodes.h>
#include <linux/slab.h>
#include <linux/poll.h>

#include "pisces.h"
#include "pisces_boot_params.h"
#include "pisces_lcall.h"
#include "pisces_xbuf.h"
#include "enclave.h"

#include "pisces_xpmem.h"

/* Longcall structures */
struct pisces_xpmem_cmd_lcall {
    struct pisces_lcall lcall;
    
    struct xpmem_cmd_ex xpmem_cmd;
    u8                  pfn_list[0];
} __attribute__((packed));

/* Ctrl command structures */
struct pisces_xpmem_cmd_ctrl {
    struct xpmem_cmd_ex xpmem_cmd;
    u8                  pfn_list[0];
} __attribute__((packed));



/* We use the XPMEM control channel to send commands */
static int
xpmem_cmd_fn(struct xpmem_cmd_ex * cmd,
             void                * priv_data)
{
    struct pisces_xpmem          * xpmem = (struct pisces_xpmem *)priv_data;
    struct pisces_xpmem_cmd_ctrl * ctrl    = NULL;
    u64                            pfn_len = 0;
    int                            status  = 0;

    if (!xpmem->connected) {
	printk(KERN_ERR "Pisces XPMEM: cannot handle command: enclave channel not connected\n");
	return -1;
    }

    if (cmd->type == XPMEM_ATTACH_COMPLETE) {
	pfn_len = cmd->attach.num_pfns * sizeof(u64);
    }

    /* Allocate memory for xpmem ctrl structure */
    ctrl = kmalloc(sizeof(struct pisces_xpmem_cmd_ctrl) + pfn_len, GFP_KERNEL);
    if (!ctrl) {
	printk(KERN_ERR "Pisces XPMEM: out of memory\n");
	return -1;
    }

    /* Copy command */
    ctrl->xpmem_cmd = *cmd;

    /* Copy pfn list */
    if (pfn_len > 0) {
	memcpy(ctrl->pfn_list, 
               cmd->attach.pfns,
	       cmd->attach.num_pfns * sizeof(u64)
	);
    }

    /* Perform xbuf send */
    status = pisces_xbuf_send(
                 xpmem->xbuf_desc, 
                 (u8 *)ctrl, 
		 sizeof(struct pisces_xpmem_cmd_ctrl) + pfn_len
	     );

    /* Free ctrl command */
    kfree(ctrl);

    return status;
}


/* Kernel initialization */
int 
pisces_xpmem_init(struct pisces_enclave * enclave)
{
    struct pisces_xpmem       * xpmem       = &(enclave->xpmem);
    struct pisces_boot_params * boot_params = NULL;

    memset(xpmem, 0, sizeof(struct pisces_xpmem));

    boot_params = __va(enclave->bootmem_addr_pa);

    /* Initialize xbuf channel */
    xpmem->xbuf_desc = pisces_xbuf_client_init(enclave, (uintptr_t)__va(boot_params->xpmem_buf_addr), 0, 0);

    if (!xpmem->xbuf_desc) {
        return -1;
    }


    /* Get xpmem partition */
    xpmem->part = xpmem_get_partition();
    if (!xpmem->part) {
	printk(KERN_ERR "Pisces XPMEM: cannot retrieve local XPMEM partition\n");
	return -1;
    }

    /* Add connection link for enclave */
    xpmem->link = xpmem_add_connection(
	    xpmem->part,
	    XPMEM_CONN_REMOTE,
	    xpmem_cmd_fn,
	    xpmem);

    if (xpmem->link <= 0) {
	printk(KERN_ERR "Pisces XPMEM: cannot create XPMEM connection\n");
	return -1;
    }

    xpmem->connected = 1;

    return 0;
}

/* Kernel deinitialization */
int
pisces_xpmem_deinit(struct pisces_enclave * enclave)
{
    struct pisces_xpmem * xpmem = &(enclave->xpmem);

    if (!xpmem->part) {
	return 0;
    }

    if (xpmem_remove_connection(xpmem->part, xpmem->link) != 0) {
	printk(KERN_ERR "XPMEM: failed to remove XPMEM connection\n");
	return -1;
    }

    return 0;
}


/* Incoming XPMEM command from enclave - copy it out and deliver to xpmem
 * partition
 */
int 
pisces_xpmem_cmd_lcall(struct pisces_enclave   * enclave, 
		       struct pisces_xbuf_desc * xbuf_desc, 
		       struct pisces_lcall     * lcall) 
{
    struct pisces_xpmem           * xpmem            = &(enclave->xpmem);
    struct pisces_xpmem_cmd_lcall * xpmem_lcall      = (struct pisces_xpmem_cmd_lcall *)lcall;
    struct xpmem_cmd_ex           * cmd              = NULL;
    struct pisces_lcall_resp        lcall_resp;
    u64                             pfn_len          = 0;

    lcall_resp.status   = 0;
    lcall_resp.data_len = 0;

    if (!xpmem->connected) {
        printk(KERN_ERR "Cannot handle enclave XPMEM request - channel not connected\n");

        lcall_resp.status = -1;
        goto out;
    }

    cmd = kmalloc(sizeof(struct xpmem_cmd_ex), GFP_KERNEL);
    if (!cmd) {
	printk(KERN_ERR "Pisces XPMEM: out of memory\n");
	lcall_resp.status = -1;
	goto out;
    }

    /*
    printk("Received %d data bytes in LCALL (xpmem_cmd_ex size: %d)\n",
	   (int)xpmem_lcall->lcall.data_len,
	   (int)sizeof(struct xpmem_cmd_ex));
    */

    /* Copy command */
    *cmd = xpmem_lcall->xpmem_cmd;

    /* Copy any pfns lists, if present */
    if (cmd->type == XPMEM_ATTACH_COMPLETE) {
	pfn_len = cmd->attach.num_pfns * sizeof(u64);
    }

    if (pfn_len > 0) {
	cmd->attach.pfns = kmalloc(pfn_len, GFP_KERNEL);
	if (!cmd->attach.pfns) {
	    printk(KERN_ERR "Pisces XPMEM: out of memory\n");
	    kfree(cmd);
	    lcall_resp.status = -1;
	    goto out;
	}

	memcpy(cmd->attach.pfns, 
	       xpmem_lcall->pfn_list, 
	       cmd->attach.num_pfns * sizeof(u64)
	);
    }
    
 out:

    /* We want to signal the end of the xbuf communication as quickly as
     * possible, because delivering the command may take some time */
    pisces_xbuf_complete(xbuf_desc, 
			 (u8 *)&lcall_resp,
			 sizeof(struct pisces_lcall_resp));

    /* Deliver command to XPMEM partition if we received everything correctly */
    if (lcall_resp.status == 0) {
	xpmem_cmd_deliver(xpmem->part, xpmem->link, cmd);

	if (pfn_len > 0) {
	    kfree(cmd->attach.pfns);
	}

	kfree(cmd);
    }

    return 0;

}
