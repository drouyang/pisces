/* 
 * Pisces Resource Control Utility
 * (c) 2014, Jack Lange <jacklange@cs.pitt.edu>
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include "pisces.h"
#include "pisces_ctrl.h"

static void usage() {
    printf("Usage: pisces_ctrl [options] <enclave_dev>\n"	\
	   " [-m, --mem=blocks]\n"	\
	   " [-c, --cpu=cores]\n"	      	\
	   " [-n, --numa=numa_zone]\n"	\
	   " [-e, --explicit]\n"		\
	   );
    exit(-1);
}


int main(int argc, char* argv[]) {
    int pisces_id = -1;
    int explicit  =  0;
    int numa_zone = -1;

    char * cpu_str = NULL;
    char * mem_str = NULL;

    int ret = 0;

    /* Parse options */
    {
	char c = 0;
	int  opt_index = 0;

	static struct option long_options[] = {
	    {"mem",      required_argument, 0, 'm'},
	    {"cpu",      required_argument, 0, 'c'},
	    {"numa",     required_argument, 0, 'n'},
	    {"explicit", no_argument,       0, 'e'},
	     {0, 0, 0, 0}
	};

	while ((c = getopt_long(argc, argv, "m:c:n:e:", long_options, &opt_index)) != -1) {
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
		case '?':
		    usage();
		    break;
	    }
	}

	if (optind + 1 != argc) {
	    usage();
	    return -1;
	} 

	pisces_id = get_pisces_id_from_path(argv[optind]);

    }


    if (mem_str) {

	if (explicit) {

	    char * iter_str = NULL;	    
	    
	    while (iter_str = strsep(&mem_str, ",")) {
		int idx = atoi(iter_str);
		if (pisces_add_mem_explicit(pisces_id, idx) == -1) {
		    printf("Error: Could not add memory block %d\n", idx);
		}
	    }
	    
	} else if (strcmp(mem_str, "all") == 0) {
	    pisces_add_mem_node(pisces_id, numa_zone);
	} else {
	    int cnt = atoi(mem_str);
	    pisces_add_mem(pisces_id, cnt, numa_zone);
	}
    }


    if (cpu_str) {
	uint64_t phys_cpu_id = 0;

	if (explicit) {
	    char * iter_str = NULL;	    

	    while (iter_str = strsep(&cpu_str, ",")) {
		phys_cpu_id = atoi(iter_str);

		if (pisces_add_cpu(pisces_id, phys_cpu_id) == -1) {
		    printf("Error: Could not add CPU to enclave %d\n", pisces_id);
		}
	    }

	} else {
	    int cnt = atoi(cpu_str);
	    pisces_add_cpus(pisces_id, cnt, numa_zone);
	}

    }

    return 0;
}
