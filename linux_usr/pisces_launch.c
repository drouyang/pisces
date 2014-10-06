#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include <pet_ioctl.h>
#include <pet_mem.h>
#include <pet_cpu.h>

#include "../src/pisces.h"

static void usage() {
    printf("Usage: pisces_launch <enclave_dev_path> "	\
	   " [-b, --block=block_id] "			\
	   " [-m, --num-blocks=N] "			\
	   " [-c, --cpu=cpu_id] "			\
	   " [-n, --numa=numa_zone]\n");
    exit(-1);
}


int main(int argc, char ** argv) {
    char * enclave_path = NULL;
    int numa_zone = -1;
    int cpu_id = -1;
    int block_id = -1;
    int num_blocks = 1;
    int ret = 0;

    struct enclave_boot_env boot_env;
    memset(&boot_env, 0, sizeof(struct enclave_boot_env));

    /* Parse options */
    {
	char c = 0;
	int opt_index = 0;

	static struct option long_options[] = {
	    {"block", required_argument, 0, 'b'},
	    {"num-blocks", required_argument, 0, 'm'},
	    {"numa", required_argument, 0, 'n'},
	    {"cpu", required_argument, 0, 'c'},
	    {0, 0, 0, 0}
	};

	while ((c = getopt_long(argc, argv, "b:m:n:c:", long_options, &opt_index)) != -1) {
	    switch (c) {
		case 'n':
		    numa_zone = atoi(optarg);
		    break;		    
		case 'c':
		    cpu_id = atoi(optarg);
		    break;
		case 'b':
		    block_id = atoi(optarg);
		    break;
		case 'm':
		    num_blocks = atoi(optarg);
		    break;
		case '?':
		    usage();
		    break;
	    }
	}

	if (optind + 1 != argc) {
	    usage();
	    return -1;
	} 

        if (num_blocks < 1) {
            printf("Error: number of memory blocks (-m) must greater than 1\n");
            return -1;
        }

	enclave_path = argv[optind];
    }



    /* Allocate N memory block from (optional) NUMA zone or as specified on cmd line*/
    {	
	if (block_id == -1) {
            struct mem_block * blocks = NULL;
            int i, ret;

            blocks = malloc(sizeof(struct mem_block) * num_blocks);
            if (blocks == NULL) {
                printf("Error: Failed to allocated memory for blocks\n");  
                return -1;
            }
            memset(blocks, 0, sizeof(struct mem_block) * num_blocks);

	    if (num_blocks > 1) {

		ret = pet_offline_contig_blocks(num_blocks, numa_zone, (1LL << 21), blocks);
		if (ret < 0) {
		    printf("Error: Could not offline %d contiguous memory blocks for enclave\n", num_blocks);
		    return -1;
		} 

		// check contiguity
		/*for (i = 1; i < num_blocks; i++) {
		    if (blocks[i].base_addr != blocks[i-1].base_addr 
			+ pet_block_size()) {
			printf("Error: Failed to allocate contigous memory blocks (block %d: %p, block %d: %p\n",
			    i - 1,
			    (void *)blocks[i - 1].base_addr,
			    i,
			    (void *)blocks[i].base_addr);
			pet_online_blocks(num_blocks, blocks);
			return -1;
		    }
		}*/
	    } else {
		ret = pet_offline_blocks(1, numa_zone, blocks);
		if (ret < 1) {
		    printf("Error: Could not offline memory block for enclave\n");
		    return -1;
		}
	    }

	    boot_env.base_addr  = blocks[0].base_addr;
	    boot_env.block_size = pet_block_size();
	    boot_env.num_blocks = num_blocks;
//	    boot_env.pages = blocks[0].pages * num_blocks;

	} else {
	    if (pet_offline_block(block_id) == -1) {
		printf("Error offlining block %d\n", block_id);
		return -1;
	    }
	    
	    boot_env.base_addr = (uint64_t)block_id * pet_block_size();
	    boot_env.block_size = pet_block_size();
	    boot_env.num_blocks = 1;
	}

        printf("Memory blocks offlined: %d\n", num_blocks);
    }


    /* Allocate 1 CPU either from the (optional) NUMA zone or as specified on cmd line */
    {
	if (cpu_id == -1) {
	    struct pet_cpu cpu;
	    memset(&cpu, 0, sizeof(struct pet_cpu));

	    if (pet_offline_cpus(1, numa_zone, &cpu) != 1) {
		printf("Error: Could not offline boot CPU for enclave\n");
		return -1;
	    }

	    boot_env.cpu_id = cpu.cpu_id;
	} else {
	    if (pet_offline_cpu(cpu_id) == -1) {
		printf("Error offlining CPU %d\n", cpu_id);
		return -1;
	    }

	    boot_env.cpu_id = cpu_id;
	}
    }
    

    printf("Launching Enclave (%s) on CPU %d\n", 
	   enclave_path, boot_env.cpu_id);

    ret = pet_ioctl_path(enclave_path, PISCES_ENCLAVE_LAUNCH, &boot_env);

    if (ret < 0) {
	printf("Error launching enclave. Code=%d\n", ret);
	return -1;
    }


    return 0;
}
