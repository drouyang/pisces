#include "pisces_lcall.h"
#include "util-hashtable.h"
#include "file_io.h"
#include "enclave.h"
#include "enclave_fs.h"

#ifdef DEBUG
static u64 xbuf_op_idx = 0;
#define debug(fmt, args...) printk(fmt, args)
#else 
#define debug(fmt, args...)
#endif


static u32 file_hash_fn(uintptr_t key) {
    return hash_long(key);
}

static int file_eq_fn(uintptr_t key1, uintptr_t key2) {
    return (key1 == key2);
}


int 
enclave_vfs_open_lcall(struct pisces_enclave   * enclave, 
		       struct pisces_xbuf_desc * xbuf_desc, 
		       struct vfs_open_lcall   * lcall) 
{
    struct enclave_fs        * fs_state = &(enclave->fs_state);
    struct file              * file_ptr = NULL;
    struct pisces_lcall_resp   vfs_resp;

    debug("Opening file %s (xbuf_desc=%p)\n", lcall->path, xbuf_desc);

    file_ptr = file_open(lcall->path, lcall->mode);

    if (IS_ERR(file_ptr)) {
	vfs_resp.status   = 0;
	vfs_resp.data_len = 0;
	pisces_xbuf_complete(xbuf_desc, (u8 *)&vfs_resp, sizeof(struct pisces_lcall_resp));
	return 0;
    }

    if (!htable_search(fs_state->open_files, (uintptr_t)file_ptr)) {
	htable_insert(fs_state->open_files, (uintptr_t)file_ptr, (uintptr_t)file_ptr);
    }

    fs_state->num_files++;

    vfs_resp.status   = (u64)file_ptr;
    vfs_resp.data_len = 0;

    debug("Returning from open (file_handle = %p)\n", (void *)file_ptr);
    pisces_xbuf_complete(xbuf_desc, (u8 *)&vfs_resp, sizeof(struct pisces_lcall_resp));

    return 0;
}


int 
enclave_vfs_close_lcall(struct pisces_enclave   * enclave, 
			struct pisces_xbuf_desc * xbuf_desc, 
			struct vfs_close_lcall  * lcall) 
{
    struct enclave_fs        * fs_state = &(enclave->fs_state);
    struct file              * file_ptr = NULL;
    struct pisces_lcall_resp   vfs_resp;

    file_ptr = (struct file *)lcall->file_handle;


    debug("closing file %p\n", file_ptr);

    
    if (!htable_search(fs_state->open_files, (uintptr_t)file_ptr)) {

	printk("File %p does not exist\n", file_ptr);

	// File does not exist
	vfs_resp.status   = -1;
	vfs_resp.data_len =  0;

	pisces_xbuf_complete(xbuf_desc, (u8 *)&vfs_resp, sizeof(struct pisces_lcall_resp));

	return 0;
    }

    htable_remove(fs_state->open_files, (uintptr_t)file_ptr, 0);
    fs_state->num_files--;

    file_close(file_ptr);

    vfs_resp.status   = 0;
    vfs_resp.data_len = 0;

    pisces_xbuf_complete(xbuf_desc, (u8 *)&vfs_resp, sizeof(struct pisces_lcall_resp));


    return 0;
}


int 
enclave_vfs_size_lcall(struct pisces_enclave   * enclave, 
		       struct pisces_xbuf_desc * xbuf_desc, 
		       struct vfs_size_lcall   * lcall) 
{
    struct enclave_fs        * fs_state = &(enclave->fs_state);
    struct file              * file_ptr = NULL;
    struct pisces_lcall_resp   vfs_resp;

    file_ptr = (struct file *)lcall->file_handle;
    
    debug("Getting file %p size\n", file_ptr);
    
    if (!htable_search(fs_state->open_files, (uintptr_t)file_ptr)) {
	printk("File %p does not exist\n", file_ptr);

	// File does not exist
	vfs_resp.status   = -1;
	vfs_resp.data_len =  0;

	pisces_xbuf_complete(xbuf_desc, (u8 *)&vfs_resp, sizeof(struct pisces_lcall_resp));
	return 0;
    }

    vfs_resp.status   = file_size(file_ptr);
    vfs_resp.data_len = 0;

    pisces_xbuf_complete(xbuf_desc, (u8 *)&vfs_resp, sizeof(struct pisces_lcall_resp));
    return 0;

}



int 
enclave_vfs_read_lcall(struct pisces_enclave   * enclave, 
		       struct pisces_xbuf_desc * xbuf_desc, 
		       struct vfs_read_lcall   * lcall)
{
    struct enclave_fs        * fs_state = &(enclave->fs_state);
    struct file              * file_ptr = NULL;
    struct pisces_lcall_resp   vfs_resp;

    u64 offset           = lcall->offset;
    u64 read_len         = lcall->length;
    u64 total_bytes_read = 0;

    u32 i = 0;

    file_ptr = (struct file *)lcall->file_handle;

    debug("FS: Reading file %p\n", file_ptr);
    
    if (!htable_search(fs_state->open_files, (uintptr_t)file_ptr)) {
	// File does not exist

	printk("File %p does not exist\n", file_ptr);

	vfs_resp.status   = -1;
	vfs_resp.data_len =  0;

	pisces_xbuf_complete(xbuf_desc, (u8 *)&vfs_resp, sizeof(struct pisces_lcall_resp));

	return 0;
    }

    for (i = 0; i < lcall->num_descs; i++) {
	struct vfs_buf_desc * desc = &(lcall->descs[i]);
	u32 desc_bytes_read        = 0;
	u32 desc_bytes_left        = desc->size;

	u64 ret = 0;
	
	if (total_bytes_read >= read_len) {
	    break;
	}

	while (desc_bytes_left > 0) {
	    ret = file_read(file_ptr, 
			    __va(desc->phys_addr + desc_bytes_read), 
			    desc_bytes_left, 
			    offset);

	    if (ret == 0) {
		break;
	    }

	    desc_bytes_read  += ret;
	    desc_bytes_left  -= ret;
	    offset           += ret;
	    total_bytes_read += ret;
	}
	
	if (ret == 0) {
	    break;
	}

    }

    
    vfs_resp.status   = total_bytes_read;
    vfs_resp.data_len = 0;

    pisces_xbuf_complete(xbuf_desc, (u8 *)&vfs_resp, sizeof(struct pisces_lcall_resp));

    return 0;
}

int 
enclave_vfs_write_lcall(struct pisces_enclave   * enclave, 
			struct pisces_xbuf_desc * xbuf_desc, 
			struct vfs_write_lcall  * lcall) 
{
    struct enclave_fs       * fs_state = &(enclave->fs_state);
    struct file             * file_ptr = NULL;
    struct pisces_lcall_resp  vfs_resp;

    u64 offset              = lcall->offset;
    u64 write_len           = lcall->length;
    u64 total_bytes_written = 0;

    u32 i = 0;

    file_ptr = (struct file *)lcall->file_handle;

    debug("writing file %p\n", file_ptr);    

    if (!htable_search(fs_state->open_files, (uintptr_t)file_ptr)) {
	// File does not exist
	printk("File %p does not exist\n", file_ptr);

	vfs_resp.status   = -1;
	vfs_resp.data_len =  0;

	pisces_xbuf_complete(xbuf_desc, (u8 *)&vfs_resp, sizeof(struct pisces_lcall_resp));

	return 0;
    }

    for (i = 0; i < lcall->num_descs; i++) {
	struct vfs_buf_desc * desc = &(lcall->descs[i]);
	u32 desc_bytes_written     = 0;
	u32 desc_bytes_left        = desc->size;
	u64 ret = 0;
	
	if (total_bytes_written >= write_len) {
	    break;
	}

	while (desc_bytes_left > 0) {
	    ret = file_write(file_ptr, 
			     __va(desc->phys_addr + desc_bytes_written), 
			     desc_bytes_left, 
			     offset);

	    if (ret == 0) {
		break;
	    }

	    desc_bytes_written  += ret;
	    desc_bytes_left     -= ret;
	    offset              += ret;
	    total_bytes_written += ret;
	}
	
	if (ret == 0) {
	    break;
	}

    }

    
    vfs_resp.status   = total_bytes_written;
    vfs_resp.data_len = 0;

    pisces_xbuf_complete(xbuf_desc, (u8 *)&vfs_resp, sizeof(struct pisces_lcall_resp));
    return 0;
}






int 
init_enclave_fs(struct pisces_enclave * enclave) 
{
    struct enclave_fs * fs_state = &(enclave->fs_state);
    fs_state->open_files         = create_htable(0, &file_hash_fn, &file_eq_fn);

    return 0;
}
