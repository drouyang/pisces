/*
 * Pisces Enclave control Utility
 * (c) Jack Lange, 2013
 * (c) Jiannan Ouyang, 2013
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pet_mem.h>
#include <pet_cpu.h>
#include <pet_types.h>

#include "pisces.h"
#include "pisces_ioctl.h"
#include "ctrl_ioctl.h"



static int 
pisces_send_ctrl_cmd(int        pisces_id,
		     uint32_t   cmd,
		     void     * argp)
{
    char * enclave_path = get_pisces_dev_path(pisces_id);
    int    ctrl_fd      = 0;
    int    ret          = 0;

    ctrl_fd = pet_ioctl_path(enclave_path, PISCES_ENCLAVE_CTRL_CONNECT, NULL);
    
    if (ctrl_fd < 0) {
	printf("Error opening enclave control channel (Pisces Enclave: %s)\n", enclave_path);
	free(enclave_path);
	return -1;
    }

    ret = pet_ioctl_fd(ctrl_fd, cmd, argp);
	
    close(ctrl_fd);
    free(enclave_path);
    return ret;
}

int 
pisces_add_mem_node(int pisces_id, 
		    int numa_zone)
{
    struct memory_range   mem_range;
    struct mem_block    * block_arr = NULL;
    int ret = 0;
    int i   = 0;
    int numa_num_blocks = pet_num_blocks(numa_zone);
    
    block_arr = calloc(numa_num_blocks, sizeof(struct mem_block));
 
    ret = pet_offline_mem_node(numa_zone, block_arr);

    for (i = 0; i < numa_num_blocks; i++) {
	mem_range.base_addr = block_arr[i].base_addr;
	mem_range.pages     = block_arr[i].pages;
	
	printf("Adding memory range (%p) to enclave %d\n", 
	       (void *)mem_range.base_addr, pisces_id);
	
	if (pisces_send_ctrl_cmd(pisces_id, ENCLAVE_CMD_ADD_MEM, &mem_range) == -1) {
	    printf("Error: Could not add memory to enclave (%d)\n", pisces_id);
	}
    }
    
    free(block_arr);
    
    return ret;
}

int 
pisces_add_mem(int pisces_id, 
	       int num_blocks, 
	       int numa_zone) 
{
    struct memory_range   mem_range;
    struct mem_block    * block_arr = NULL;
    int i   = 0;
    int ret = 0;
    
    block_arr = calloc(num_blocks, sizeof(struct mem_block));
    ret       = pet_offline_blocks(num_blocks, numa_zone, block_arr);
    
    if (ret != num_blocks) {
	printf("Error: Could not allocate %d memory blocks\n", num_blocks);
	
	pet_online_blocks(ret, block_arr);
	free(block_arr);
	
	return -1;
    }
    
    for (i = 0; i < num_blocks; i++) {
	mem_range.base_addr = block_arr[i].base_addr;
	mem_range.pages     = block_arr[i].pages;
	
	printf("Adding memory range (%p) to enclave %d\n", 
	       (void *)mem_range.base_addr, pisces_id);
	
	if (pisces_send_ctrl_cmd(pisces_id, ENCLAVE_CMD_ADD_MEM, &mem_range) == -1) {
	    printf("Error: Could not add memory to enclave (%d)\n", pisces_id);
	}
    }
    
    free(block_arr);
    
    return 0;
}
	       
int 
pisces_add_mem_explicit(int pisces_id,
			int block_id)
{
    struct memory_range mem_range;
  
    if (pet_offline_block(block_id) == -1) {
	printf("Error: Could not offline memory block %d\n", block_id);
	return -1;
    }

    mem_range.base_addr = pet_block_size() * block_id;
    mem_range.pages     = pet_block_size() / 4096;
    
    if (pisces_send_ctrl_cmd(pisces_id, ENCLAVE_CMD_ADD_MEM, &mem_range) == -1) {
	printf("Error: Could not add explicit memory block (block_id: %d)\n", block_id); 
	pet_online_block(block_id);
	return -1;
    }
    
    return 0;

}


int 
pisces_add_cpus(int pisces_id,
		int num_cpus, 
		int numa_zone)
{
    struct pet_cpu * cpu_arr = NULL;
    uint64_t         cpu_id  = 0;
    int i   = 0;
    int ret = 0;
    
    cpu_arr = calloc(num_cpus, sizeof(struct pet_cpu));
    ret     = pet_offline_cpus(num_cpus, numa_zone, cpu_arr);
    
    if (ret != num_cpus) {
	printf("Error: Could not allocate %d CPUs\n", num_cpus);
	
	pet_online_cpus(ret, cpu_arr);
	free(cpu_arr);
	
	return -1;
    }
    
    for (i = 0; i < num_cpus; i++) {
	cpu_id = cpu_arr[i].cpu_id;
	
	printf("Adding CPU %d to enclave %d\n", 
	       (void *)cpu_id, pisces_id);
	
	if (pisces_send_ctrl_cmd(pisces_id, ENCLAVE_CMD_ADD_CPU, (void *)cpu_id) != 0) {
	    printf("Error: Could not add CPU %d to enclave %d\n", cpu_id, pisces_id);
	    pet_online_cpu(cpu_id);
	    continue;
	}
    }
    
    free(cpu_arr);
    
    
    return 0;
}


int 
pisces_add_cpu(int      pisces_id,
	       uint64_t cpu_id)
{

    if (pet_offline_cpu(cpu_id) == -1) {
	printf("Error: Could not offline CPU %d\n", cpu_id);
	return -1;
    }
    

    if (pisces_send_ctrl_cmd(pisces_id, ENCLAVE_CMD_ADD_CPU, (void *)cpu_id) != 0) {
	printf("Error: Could not add CPU %llu to enclave\n", cpu_id);
	pet_online_cpu(cpu_id);
	return -1;
    }

    return 0;
}


int 
pisces_remove_cpu(int      pisces_id,
		  uint64_t cpu_id)
{
    if (pisces_send_ctrl_cmd(pisces_id, ENCLAVE_CMD_REMOVE_CPU, (void *)cpu_id) != 0) {
	printf("Error: Could not remove CPU %llu from enclave\n", cpu_id);
	return -1;
    }

    pet_online_cpu(cpu_id);

    return 0;
}



int
pisces_run_job(int         pisces_id,
	       char      * name, 
	       char      * exe_path,
	       char      * exe_argv,
	       char      * envp,
	       uint8_t     use_large_pages,
	       uint8_t     use_smartmap,
	       uint8_t     num_ranks,
	       uint64_t    cpu_mask,
	       uint64_t    heap_size,
	       uint64_t    stack_size)
{
    struct pisces_job_spec job_spec;

    memset(&job_spec, 0, sizeof(struct pisces_job_spec));

    if ((name     == NULL) || 
	(exe_path == NULL)) {
	printf("Error: Missing name/exe parameter\n"); 
	return -1;
    }

    if (strlen(name) > 63) {
	printf("Error: Name is too large (MAX=63)\n");
	return -1;
    }

    if (strlen(exe_path) > 255) {
	printf("Error: exe path is too large (MAX=255)\n");
	return -1;
    }

    strncpy(job_spec.name,     name,     63);
    strncpy(job_spec.exe_path, exe_path, 255);
    
    if (exe_argv) {

	if (strlen(exe_argv) > 255) {
	    printf("Error: ARGV string is too large (MAX SIZE=255)\n");
	    return -1;
	}

	strncpy(job_spec.argv, exe_argv, 255);
    }

    if (envp) {
	
	if (strlen(envp) > 255) {
	    printf("Error: ENVP string is too large (MAX=255)\n");
	    return -1;
	}

	strncpy(job_spec.envp, envp, 255);
    }


    job_spec.use_large_pages = use_large_pages;
    job_spec.use_smartmap    = use_smartmap;
    job_spec.num_ranks       = num_ranks;
    job_spec.cpu_mask        = cpu_mask;
    job_spec.heap_size       = heap_size;
    job_spec.stack_size      = stack_size;


    return pisces_send_ctrl_cmd(pisces_id, ENCLAVE_CMD_LAUNCH_JOB, &job_spec);
}
