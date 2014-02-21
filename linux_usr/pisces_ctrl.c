/*
 * Pisces Enclave control Utility
 * (c) Jack Lange, 2013
 * (c) Jiannan Ouyang, 2013
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>


#include <pet_mem.h>
#include <pet_ioctl.h>
#include <pet_cpu.h>

#include "../src/pisces.h"
#include "../src/pisces_cmds.h"

static void usage() {
    printf("Usage: pisces_ctrl [options] <enclave_dev>\n"	\
	   " [-m, --mem=blocks]\n"	\
	   " [-c, --cpu=cores]\n"	      	\
	   " [-n, --numa=numa_zone]\n"	\
	   " [-e, --explicit]\n"		\
	   " [-r, --remove: remove explicitly]\n"\
	   );
    exit(-1);
}


int main(int argc, char* argv[]) {
    char * enclave_path = NULL;
    int ctrl_fd = 0;

    int explicit = 0;
    int remove = 0;
    int numa_zone = -1;

    char * cpu_str = NULL;
    char * mem_str = NULL;

    int ret = 0;

    /* Parse options */
    {
	char c = 0;
	int opt_index = 0;

	static struct option long_options[] = {
	    {"mem", required_argument, 0, 'm'},
	    {"cpu", required_argument, 0, 'c'},
	    {"numa", required_argument, 0, 'n'},
	    {"explicit", no_argument, 0, 'e'},
	    {"remove", no_argument, 0, 'r'},
	     {0, 0, 0, 0}
	};

	while ((c = getopt_long(argc, argv, "m:c:n:e:r", long_options, &opt_index)) != -1) {
	    switch (c) {
		case 'm':
		    mem_str = optarg;
		    break;
		case 'c':
		    cpu_str = optarg;
		    break;
		case 'n':
		    numa_zone = atoi(optarg);
		    break;		    
		case 'e':
		    explicit = 1;
		    break;
                case 'r':
                    remove = 1;
                    explicit = 1;
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

	enclave_path = argv[optind];

    }


    ctrl_fd = pet_ioctl_path(enclave_path, PISCES_ENCLAVE_CTRL_CONNECT, NULL);

    if (ctrl_fd < 0) {
	printf("Error opening enclave control channel (%s)\n", enclave_path);
	return -1;
    }


    if (mem_str) {
	struct memory_range mem_range;

	if (explicit) {
	    char * iter_str = NULL;	    

	    while (iter_str = strsep(&mem_str, ",")) {
		int idx = atoi(iter_str);

		if (pet_offline_block(idx) == -1) {
		    printf("Error: Could not offline memory block %d\n", idx);
		    continue;
		}

		mem_range.base_addr = idx * pet_block_size();
		mem_range.pages = pet_block_size() / 4096;

		if (pet_ioctl_fd(ctrl_fd, ENCLAVE_CMD_ADD_MEM, &mem_range) != 0) {
		    printf("Error: Could not add memory block %d to enclave\n", idx);
		    continue;
		}
	    }

	} else {
	    struct mem_block * block_arr = NULL;
	    int cnt = 0;
	    int i = 0;
	    int ret = 0;
	    int numa_num_blocks = pet_num_blocks(numa_zone);

	    if (strcmp(mem_str, "all") == 0) {
		cnt = numa_num_blocks;
	    } else {
		cnt = atoi(mem_str);
	    }

	    block_arr = malloc(sizeof(struct mem_block) * cnt);
	    memset(block_arr, 0, sizeof(struct mem_block) * cnt);

	    if ((cnt == numa_num_blocks) && (strcmp(mem_str, "all") == 0)) {
		ret = pet_offline_node(numa_zone, block_arr);
		cnt = ret;
	    } else {
		ret = pet_offline_blocks(cnt, numa_zone, block_arr);
	    }

	    if (ret != cnt) {
		printf("Error: Could not allocate %d memory blocks\n", cnt);

		pet_online_blocks(ret, block_arr);
		free(block_arr);

		return -1;
	    }
	    
	    for (i = 0; i < cnt; i++) {
		mem_range.base_addr = block_arr[i].base_addr;
		mem_range.pages     = block_arr[i].pages;

		printf("Adding memory range (%p) to enclave %s\n", 
		       (void *)mem_range.base_addr, enclave_path);

		if (pet_ioctl_fd(ctrl_fd, ENCLAVE_CMD_ADD_MEM, &mem_range) != 0) {
		    printf("Error: Could not add memory block %d to enclave\n", block_arr[i].base_addr / pet_block_size());
		    continue;
		}
	    }

	    free(block_arr);
	}
    }


    if (cpu_str) {
	uint64_t phys_cpu_id = 0;

	if (explicit) {
	    char * iter_str = NULL;	    

	    while (iter_str = strsep(&cpu_str, ",")) {
		phys_cpu_id = atoi(iter_str);

                if (!remove) {
                    if (pet_offline_cpu(phys_cpu_id) == -1) {
                        printf("Error: Could not offline CPU %d\n", phys_cpu_id);
                        continue;
                    }
           
                    if (pet_ioctl_fd(ctrl_fd, ENCLAVE_CMD_ADD_CPU, (void *)phys_cpu_id) != 0) {
                        printf("Error: Could not add CPU %llu to enclave\n", phys_cpu_id);
                        continue;
                    }
                } else {
                    if (pet_ioctl_fd(ctrl_fd, ENCLAVE_CMD_REMOVE_CPU, (void *)phys_cpu_id) != 0) {
                        printf("Error: Could not remove CPU %llu from enclave\n", phys_cpu_id);
                        continue;
                    }
                }
	    }

	} else {
	    struct pet_cpu * cpu_arr = NULL;
	    int cnt = atoi(cpu_str);
	    int i = 0;
	    int ret = 0;

	    cpu_arr = malloc(sizeof(struct pet_cpu) * cnt);
	    memset(cpu_arr, 0, sizeof(struct pet_cpu) * cnt);

	    ret = pet_offline_cpus(cnt, numa_zone, cpu_arr);

	    if (ret != cnt) {
		printf("Error: Could not allocate %d CPUs\n", cnt);

		pet_online_cpus(ret, cpu_arr);
		free(cpu_arr);

		return -1;
	    }
	    
	    for (i = 0; i < cnt; i++) {
		phys_cpu_id = cpu_arr[i].cpu_id;

		printf("Adding CPU %d to enclave %s\n", 
		       (void *)phys_cpu_id, enclave_path);

		if (pet_ioctl_fd(ctrl_fd, ENCLAVE_CMD_ADD_CPU, (void *)phys_cpu_id) != 0) {
		    printf("Error: Could not add CPU %d to enclave\n", phys_cpu_id);
		    continue;
		}
	    }

	    free(cpu_arr);
	}

    }


    close(ctrl_fd);

    return 0;
}
