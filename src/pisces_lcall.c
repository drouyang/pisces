/* 
 * Remote system call implementation 
 * (c) Jack Lange, 2013 (jacklange@cs.pitt.edu)
 */

#include <linux/types.h>
#include <linux/sched.h>
#include <linux/anon_inodes.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/kthread.h>

#include "pisces.h"
#include "pisces_boot_params.h"
#include "pisces_lcall.h"
#include "pisces_pci.h"
#include "enclave.h"
#include "boot.h"
#include "util-hashtable.h"
#include "enclave_fs.h"
#include "ipi.h"
#include "pisces_xbuf.h"



static void lcall_handler(struct pisces_enclave * enclave, struct pisces_xbuf_desc * lcall_desc) {
    struct pisces_lcall_state * lcall_state = &(enclave->lcall_state);
    struct pisces_xbuf_desc * xbuf_desc = lcall_state->xbuf_desc;
    
    if (pisces_xbuf_pending(xbuf_desc)) {
	lcall_state->active_lcall = 1;

	printk("Waking up kernel thread for lcall (xbuf_desc = %p)\n", xbuf_desc);
	wake_up_interruptible(&(lcall_state->kern_waitq));
    }
    
    return;
}

static int lcall_kern_thread(void * arg) {
    struct pisces_enclave * enclave = arg;
    struct pisces_lcall_state * lcall_state = &(enclave->lcall_state);
    struct pisces_xbuf_desc * xbuf_desc = lcall_state->xbuf_desc;
    struct pisces_lcall_resp resp;
    struct pisces_lcall * cur_lcall = NULL;
    u32 lcall_size = 0;
	
    
    while (1) {
        //  printk("LCALL Kernel thread going to sleep on cmd buf\n");
        wait_event_interruptible(lcall_state->kern_waitq, 
				 (lcall_state->active_lcall == 1));
	
	
	// grab the lcall from the xbuf
	pisces_xbuf_recv(xbuf_desc, (u8 **)&cur_lcall, &lcall_size);
	
	printk("kernel thread is awake (handling lcall %llu)\n", cur_lcall->lcall);

        switch (cur_lcall->lcall) {
            case PISCES_LCALL_VFS_READ:
                enclave_vfs_read_lcall(enclave, xbuf_desc, (struct vfs_read_lcall *)cur_lcall);
                break;
            case PISCES_LCALL_VFS_WRITE:
                enclave_vfs_write_lcall(enclave, xbuf_desc, (struct vfs_write_lcall *)cur_lcall);
                break;
            case PISCES_LCALL_VFS_OPEN: 
                enclave_vfs_open_lcall(enclave, xbuf_desc, (struct vfs_open_lcall *)cur_lcall);
                break;
            case PISCES_LCALL_VFS_CLOSE:
                enclave_vfs_close_lcall(enclave, xbuf_desc, (struct vfs_close_lcall *)cur_lcall);
                break;
            case PISCES_LCALL_VFS_SIZE:
                enclave_vfs_size_lcall(enclave, xbuf_desc, (struct vfs_size_lcall *)cur_lcall);
                break;
            case PISCES_LCALL_PCI_SETUP:
                enclave_pci_setup_lcall(enclave, xbuf_desc, cur_lcall);
                break;
#ifdef PORTALS
            case PISCES_LCALL_XPMEM_VERSION:
                pisces_portals_xpmem_version(enclave, xbuf_desc, cur_lcall);
                break;
            case PISCES_LCALL_XPMEM_MAKE:
                pisces_portals_xpmem_make(enclave, xbuf_desc, cur_lcall);
                break;
            case PISCES_LCALL_XPMEM_REMOVE:
                pisces_portals_xpmem_remove(enclave, xbuf_desc, cur_lcall);
                break;
            case PISCES_LCALL_XPMEM_GET:
                pisces_portals_xpmem_get(enclave, xbuf_desc, cur_lcall);
                break;
            case PISCES_LCALL_XPMEM_RELEASE:
                pisces_portals_xpmem_release(enclave, xbuf_desc, cur_lcall);
                break;
            case PISCES_LCALL_XPMEM_ATTACH:
                pisces_portals_xpmem_attach(enclave, xbuf_desc, cur_lcall);
                break;
            case PISCES_LCALL_XPMEM_DETACH:
                pisces_portals_xpmem_detach(enclave, xbuf_desc, cur_lcall);
                break;

#endif
            case PISCES_LCALL_VFS_READDIR:
            default:
                printk(KERN_ERR "Enclave requested unimplemented LCALL %llu\n", cur_lcall->lcall);
                resp.status = -1;
                resp.data_len = 0;
		pisces_xbuf_complete(xbuf_desc, (u8*)&resp, sizeof(struct pisces_lcall_resp));
                break;
        }
	
	kfree(cur_lcall);
	lcall_state->active_lcall = 0;

    }


    return 0;
}


int
pisces_lcall_init( struct pisces_enclave * enclave) {
    struct pisces_lcall_state * lcall_state = &(enclave->lcall_state);
    struct pisces_boot_params * boot_params = NULL;
    struct pisces_xbuf_desc * xbuf_desc = NULL;

    init_waitqueue_head(&lcall_state->kern_waitq);

    boot_params = __va(enclave->bootmem_addr_pa);

    xbuf_desc = pisces_xbuf_server_init(enclave, (uintptr_t)__va(boot_params->longcall_buf_addr), 
			    boot_params->longcall_buf_size, 
			    lcall_handler, STUPID_LINUX_IRQ, apic->cpu_present_to_apicid(0));

    if (xbuf_desc == NULL) {
	printk("Error initializing LCALL XBUF server\n");
	return -1;
    }

    lcall_state->xbuf_desc = xbuf_desc;
    lcall_state->active_lcall = 0;
    {
	char thrd_name[32];
	memset(thrd_name, 0, 32);
	
	snprintf(thrd_name, 32, "enclave%d-lcalld", enclave->id);
	
	lcall_state->kern_thread = kthread_create(lcall_kern_thread, enclave, thrd_name);
	wake_up_process(lcall_state->kern_thread);
    }


    return 0;
}


