#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include <pet_ioctl.h>
#include <pet_mem.h>
#include <pet_cpu.h>

#include "pisces.h"

static void usage() {
    printf("Usage: pisces_launch <enclave_dev_path> "	\
	   " [-b, --block=block_id] "			\
	   " [-m, --num-blocks=N] "			\
	   " [-c, --cpu=cpu_id] "			\
	   " [-n, --numa=numa_zone]\n");
    exit(-1);
}


int 
main(int argc, char ** argv) 
{
    int    numa_zone    = PISCES_ANY_NUMA_ZONE;
    int    cpu_id       = PISCES_ANY_CPU;
    int    block_id     = PISCES_ANY_MEMBLOCK;
    int    num_blocks   =  1;
    int    enclave_id   = -1;


    /* Parse options */
    {
	char c = 0;
	int  opt_index = 0;

	static struct option long_options[] = {
	    {"block",      required_argument, 0, 'b'},
	    {"num-blocks", required_argument, 0, 'm'},
	    {"numa",       required_argument, 0, 'n'},
	    {"cpu",        required_argument, 0, 'c'},
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

	enclave_id = get_pisces_id_from_path(argv[optind]);

	if (enclave_id == -1) {
	    printf("Invalid Enclave Requested (%s)\n", argv[optind]);
	    return -1;
	}
    }
    
    if (pisces_launch(enclave_id, cpu_id, block_id, numa_zone, num_blocks) != 0) {
	printf("Error: Could not launch enclave %d\n", enclave_id);
	return -1;
    }

    
    return 0;

  
}
