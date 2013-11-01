#include "pisces_lcall.h"
#include "util-hashtable.h"
#include "file_io.h"
#include "pisces_cmds.h"
#include "enclave.h"


static u32 file_hash_fn(uintptr_t key) {
    return hash_long(key);
}

static int file_eq_fn(uintptr_t key1, uintptr_t key2) {
    return (key1 == key2);
}


int enclave_vfs_open_lcall(struct pisces_enclave * enclave, 
			   struct pisces_cmd_buf * cmd_buf) {
    struct enclave_fs * fs_state = &(enclave->fs_state);
    struct vfs_open_cmd * cmd = (struct vfs_open_cmd *)&(cmd_buf->cmd);
    struct pisces_resp * resp = &(cmd_buf->resp);
    struct file * file_ptr = NULL;

    printk("Opening file %s\n", cmd->path);

    file_ptr = file_open(cmd->path, cmd->mode);

    if (IS_ERR(file_ptr)) {
	resp->status = 0;
	resp->data_len = 0;
	return 0;
    }

    if (!htable_search(fs_state->open_files, (uintptr_t)file_ptr)) {
	htable_insert(fs_state->open_files, (uintptr_t)file_ptr, (uintptr_t)file_ptr);
    }

    fs_state->num_files++;

    resp->status = (u64)file_ptr;
    resp->data_len = 0;

    printk("Returning from open (file_handle = %p)\n", (void *)file_ptr);

    return 0;
}


int enclave_vfs_close_lcall(struct pisces_enclave * enclave, 
			    struct pisces_cmd_buf * cmd_buf) {
    struct enclave_fs * fs_state = &(enclave->fs_state);
    struct vfs_close_cmd * cmd = (struct vfs_close_cmd *)&(cmd_buf->cmd);
    struct pisces_resp * resp = &(cmd_buf->resp);
    struct file * file_ptr = NULL;

    file_ptr = (struct file *)cmd->file_handle;


    printk("closing file %p\n", file_ptr);

    
    if (!htable_search(fs_state->open_files, (uintptr_t)file_ptr)) {
	printk("File %p does not exist\n", file_ptr);
	// File does not exist
	resp->status = -1;
	resp->data_len = 0;
	return 0;
    }

    htable_remove(fs_state->open_files, (uintptr_t)file_ptr, 0);
    fs_state->num_files--;

    file_close(file_ptr);

    resp->status = 0;
    resp->data_len = 0;

    return 0;
}


int enclave_vfs_size_lcall(struct pisces_enclave * enclave, 
			   struct pisces_cmd_buf * cmd_buf) {
    struct enclave_fs * fs_state = &(enclave->fs_state);
    struct vfs_size_cmd * cmd = (struct vfs_size_cmd *)&(cmd_buf->cmd);
    struct pisces_resp * resp = &(cmd_buf->resp);
    struct file * file_ptr = NULL;

    file_ptr = (struct file *)cmd->file_handle;
    
    printk("Getting file %p size\n", file_ptr);
    
    if (!htable_search(fs_state->open_files, (uintptr_t)file_ptr)) {
	printk("File %p does not exist\n", file_ptr);

	// File does not exist
	resp->status = -1;
	resp->data_len = 0;
	return 0;
    }

    resp->status = file_size(file_ptr);

    resp->data_len = 0;

    return 0;

}



int enclave_vfs_read_lcall(struct pisces_enclave * enclave, 
			   struct pisces_cmd_buf * cmd_buf) {
    struct enclave_fs * fs_state = &(enclave->fs_state);
    struct vfs_read_cmd * cmd = (struct vfs_read_cmd *)&(cmd_buf->cmd);
    struct pisces_resp * resp = &(cmd_buf->resp);
    struct file * file_ptr = NULL;
    u64 offset = cmd->offset;
    u64 read_len = cmd->length;
    u64 total_bytes_read = 0;
    u32 i = 0;

    file_ptr = (struct file *)cmd->file_handle;
    printk("reading file %p\n", file_ptr);
    
    if (!htable_search(fs_state->open_files, (uintptr_t)file_ptr)) {
	// File does not exist
	printk("File %p does not exist\n", file_ptr);

	resp->status = -1;
	resp->data_len = 0;
	return 0;
    }

    for (i = 0; i < cmd->num_descs; i++) {
	struct vfs_buf_desc * desc = &(cmd->descs[i]);
	u32 desc_bytes_read = 0;
	u32 desc_bytes_left = desc->size;
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

	    desc_bytes_read += ret;
	    desc_bytes_left -= ret;
	    offset += ret;
	    total_bytes_read += ret;
	}
	
	if (ret == 0) {
	    break;
	}

    }

    
    resp->status = total_bytes_read;
    resp->data_len = 0;

    return 0;
}

int enclave_vfs_write_lcall(struct pisces_enclave * enclave, 
			    struct pisces_cmd_buf * cmd_buf) {
    struct enclave_fs * fs_state = &(enclave->fs_state);
    struct vfs_write_cmd * cmd = (struct vfs_write_cmd *)&(cmd_buf->cmd);
    struct pisces_resp * resp = &(cmd_buf->resp);
    struct file * file_ptr = NULL;
    u64 offset = cmd->offset;
    u64 write_len = cmd->length;
    u64 total_bytes_written = 0;
    u32 i = 0;

    file_ptr = (struct file *)cmd->file_handle;
    printk("writing file %p\n", file_ptr);    

    if (!htable_search(fs_state->open_files, (uintptr_t)file_ptr)) {
	// File does not exist
	printk("File %p does not exist\n", file_ptr);

	resp->status = -1;
	resp->data_len = 0;
	return 0;
    }

    for (i = 0; i < cmd->num_descs; i++) {
	struct vfs_buf_desc * desc = &(cmd->descs[i]);
	u32 desc_bytes_written = 0;
	u32 desc_bytes_left = desc->size;
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

	    desc_bytes_written += ret;
	    desc_bytes_left -= ret;
	    offset += ret;
	    total_bytes_written += ret;
	}
	
	if (ret == 0) {
	    break;
	}

    }

    
    resp->status = total_bytes_written;
    resp->data_len = 0;

    return 0;
}






int init_enclave_fs(struct pisces_enclave * enclave) {
    struct enclave_fs * fs_state = &(enclave->fs_state);
    
    fs_state->open_files = create_htable(0, &file_hash_fn, &file_eq_fn);

    return 0;
}
