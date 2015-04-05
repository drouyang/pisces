/* Pisces Interface Library 
 * (c) 2014, Jack Lange <jacklange@cs.pitt.edu>
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "pisces.h"

#include "pisces_ioctl.h"
#include "ctrl_ioctl.h"
#include <pet_ioctl.h>

#include <pet_mem.h>
#include <pet_cpu.h>

int
pisces_load(char * kern, 
	    char * initrd, 
	    char * cmd_line) 
{
    int kern_fd    = 0;
    int init_fd    = 0;
    int enclave_id = 0;

    struct pisces_image * img = NULL;


    kern_fd = open(kern, O_RDONLY);
    if (kern_fd == -1) {
	printf("Error: Could not find valid kernel image at (%s)\n", kern);
	return -1;
    }

    init_fd = open(initrd, O_RDONLY);
    if (init_fd == -1) {
	printf("Error: Could not find valid initrd file at (%s)\n", initrd);
	return -1;
    }

    img = malloc(sizeof(struct pisces_image));

    img->kern_fd    = kern_fd;
    img->init_fd    = init_fd;
    strncpy(img->cmd_line, cmd_line, 1023);

    enclave_id = pet_ioctl_path("/dev/" DEVICE_NAME, PISCES_LOAD_IMAGE, img);
    
    if (enclave_id < 0 ) {
	printf("Error: Could not create enclave (Error Code: %d)\n", enclave_id);
	return -1;
    }


    return enclave_id;
}


int
pisces_launch(int pisces_id, 
	      int cpu_id, 
	      int block_id, 
	      int numa_zone, 
	      int num_blocks)
{
    char * enclave_path = get_pisces_dev_path(pisces_id);
    int    ret          = 0;

    struct enclave_boot_env boot_env;

    memset(&boot_env, 0, sizeof(struct enclave_boot_env));



    if (enclave_path == NULL) {
	return -1;
    }

    /* Allocate N memory block from (optional) NUMA zone or as specified on cmd line*/
    {	
	if (block_id == PISCES_ANY_MEMBLOCK) {
            struct mem_block * blocks = NULL;
            int ret = 0;

            blocks = malloc(sizeof(struct mem_block) * num_blocks);

            if (blocks == NULL) {
                printf("Error: Failed to allocated memory for blocks\n");  
                goto mem_error;
            }

            memset(blocks, 0, sizeof(struct mem_block) * num_blocks);


	    ret = pet_offline_contig_blocks(num_blocks, numa_zone, (1LL << 21), blocks);
	    
	    if (ret < 0) {
		printf("Error: Could not offline %d contiguous memory blocks for enclave\n", num_blocks);
		free(blocks);
		goto mem_error;
	    } 
	    
	

	    boot_env.base_addr  = blocks[0].base_addr;
	    boot_env.block_size = pet_block_size();
	    boot_env.num_blocks = num_blocks;

	    free(blocks);

	} else {
	    if (pet_offline_block(block_id) == -1) {
		printf("Error offlining block %d\n", block_id);
		goto mem_error;
	    }
	    
	    boot_env.base_addr  = (uint64_t)block_id * pet_block_size();
	    boot_env.block_size = pet_block_size();
	    boot_env.num_blocks = 1;
	}

        printf("Memory blocks offlined: %d\n", num_blocks);
    }


    /* Allocate 1 CPU either from the (optional) NUMA zone or as specified on cmd line */
    {
	if (cpu_id == PISCES_ANY_CPU) {
	    struct pet_cpu cpu;
	    memset(&cpu, 0, sizeof(struct pet_cpu));

	    if (pet_offline_cpus(1, numa_zone, &cpu) != 1) {
		printf("Error: Could not offline boot CPU for enclave\n");
		goto cpu_error;
	    }

	    boot_env.cpu_id = cpu.cpu_id;
	} else {
	    if (pet_offline_cpu(cpu_id) == -1) {
		printf("Error offlining CPU %d\n", cpu_id);
		goto cpu_error;
	    }

	    boot_env.cpu_id = cpu_id;
	}
    }
    

    printf("Launching Enclave (%s) on CPU %d\n", 
	   enclave_path, boot_env.cpu_id);

    ret = pet_ioctl_path(enclave_path, PISCES_ENCLAVE_LAUNCH, &boot_env);

    if (ret < 0) {
	printf("Error launching enclave. Code=%d\n", ret);
	goto ioctl_error;
    }


    free(enclave_path);
    return 0;

 ioctl_error:
    pet_online_cpu(boot_env.cpu_id);
 cpu_error:
    pet_online_contig_blocks(boot_env.num_blocks, boot_env.base_addr);    
 mem_error:
    free(enclave_path);
    return -1;
}


int 
pisces_get_cons_fd(int pisces_id)
{
    char * enclave_path = get_pisces_dev_path(pisces_id);
    int    fd = -1;

    fd = pet_ioctl_path(enclave_path, PISCES_ENCLAVE_CONS_CONNECT, NULL);
  
    free(enclave_path);

    return fd;
}




int 
pisces_reset(int pisces_id)
{
    char * enclave_path = get_pisces_dev_path(pisces_id);
    
    pet_ioctl_path(enclave_path, PISCES_ENCLAVE_RESET, NULL);

    return 0;
}
