/* Host PCI User space tool
 *  (c) Jack Lange, 2012
 *  jacklange@cs.pitt.edu 
 */
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h> 
#include <sys/ioctl.h> 
#include <sys/stat.h> 
#include <sys/types.h> 
#include <unistd.h>
#include <string.h>
#include <getopt.h>


#include <pet_pci.h>
#include <pet_ioctl.h>

#include "../src/pisces.h"
#include "../src/pisces_cmds.h"



void usage() {
    printf("Usage:\n"); 
    printf("\tv3_pci [<bus>:<dev>.<fn>]                      --- List PCI Device State\n");
    printf("\tv3_pci -a <name> <bus>:<dev>.<fn> <enclave>    --- Add PCI Device\n");
    printf("\tv3_pci -r <name> <bus>:<dev>.<fn> <enclave>    --- Remove PCI Device\n");
}


typedef enum {QUERY, ADD, REMOVE} op_mode_t;



int main(int argc, char ** argv) {
    char      * bdf_str = NULL;
    char      * name    = NULL;
    op_mode_t   mode    = QUERY;


    {
	char c         = 0;
	int  opt_index = 0;

	static struct option long_options[] = {
	    {"help",   no_argument,       0, 'h'},
	    {"remove", required_argument, 0, 'r'},
	    {"add",    required_argument, 0, 'a'},
	    {0, 0, 0, 0}
	};

	while ((c = getopt_long(argc, argv, "r:a:h", long_options, &opt_index)) != -1) {
	    switch (c) {
		case 'r':
		    mode = REMOVE;
		    name = optarg;
		    break;
		case 'a':
		    mode = ADD;
		    name = optarg;
		    break;
		case 'h':
		case '?':
		    usage();
		    return -1;
	    }
	}
    }

    if (mode == QUERY) {
	
	if (argc == 1) {
	    unsigned int     num_devs = 0;
	    struct pet_pci * pci_arr  = NULL;
	    int j = 0;
	    
	    if (pet_probe_pci(&num_devs, &pci_arr) != 0) {
		printf("Error: Could not probe PCI\n");
	    } else {
		printf("PCI Device States:\n");

		for (j = 0; j < num_devs; j++) {
		    printf("%.2x:%.2x.%u --  %s\n", 
			   pci_arr[j].bus, pci_arr[j].dev, pci_arr[j].fn,
			   pet_pci_state_to_str(pci_arr[j].state));
		}
	    }
	    
	} else if (argc == 2) {
	    unsigned int bus = 0;
	    unsigned int dev = 0;
	    unsigned int fn  = 0;
	    
	    bdf_str = argv[1];
	    
	    if (pet_parse_bdf(bdf_str, &bus, &dev, &fn) != 0) {
		printf("Error: Could not parse BDF spcification string\n");
		return -1;
	    }
	    
	    printf("Status=%s\n", pet_pci_state_to_str(pet_pci_status(bus, dev, fn)));
	    
	} else {
	    usage();
	    return -1;
	}
    } else if (mode == ADD)   {
	unsigned int bus = 0;
	unsigned int dev = 0;
	unsigned int fn  = 0;

	struct pisces_pci_spec dev_spec;

	char * enclave_path = NULL;
	int    ctrl_fd      = 0;

	if (argc - optind + 1 < 3) {
	    usage();
	    return -1;
	}
	

	bdf_str      = argv[optind];
	enclave_path = argv[optind + 1];
	
	if (pet_parse_bdf(bdf_str, &bus, &dev, &fn) != 0) {
	    printf("Error: Could not parse BDF spcification string\n");
	    return -1;
	}
	


	if (pet_offline_pci(bus, dev, fn) != 0) {
	    printf("Error: Could not offline PCI device\n");
	    return -1;
	}
	
	memset(&dev_spec, 0, sizeof(struct pisces_pci_spec));
	
	
	dev_spec.bus  = bus;
	dev_spec.dev  = dev;
	dev_spec.func = fn;
	strncpy(dev_spec.name, name, 128);

	ctrl_fd = pet_ioctl_path(enclave_path, PISCES_ENCLAVE_CTRL_CONNECT, NULL);

	if (ctrl_fd < 0) {
	    printf("Error opening enclave control channel (%s)\n", enclave_path);
	    pet_online_pci(bus, dev, fn);
	    return -1;
	}
	
	if (pet_ioctl_fd(ctrl_fd, ENCLAVE_CMD_ADD_V3_PCI,  &dev_spec) != 0) {
	    printf("Error: Could not add device to Palacios\n");
	    pet_online_pci(bus, dev, fn);
	    return -1;
	}

    } else if (mode == REMOVE) {
	unsigned int bus = 0;
	unsigned int dev = 0;
	unsigned int fn  = 0;
	struct pisces_pci_spec dev_spec;
	
	char * enclave_path = NULL;
	int    ctrl_fd      = 0;


	if (argc - optind + 1 < 3) {
	    usage();
	    return -1;
	}
	
	bdf_str      = argv[optind];
	enclave_path = argv[optind + 1];
	

	if (pet_parse_bdf(bdf_str, &bus, &dev, &fn) != 0) {
	    printf("Error: Could not parse BDF spcification string (%s)\n", bdf_str);
	    return -1;
	}
	

	memset(&dev_spec, 0, sizeof(struct pisces_pci_spec));
	
	
	dev_spec.bus  = bus;
	dev_spec.dev  = dev;
	dev_spec.func = fn;
	strncpy(dev_spec.name, name, 128);
	
	ctrl_fd = pet_ioctl_path(enclave_path, PISCES_ENCLAVE_CTRL_CONNECT, NULL);

	if (ctrl_fd < 0) {
	    printf("Error opening enclave control channel (%s)\n", enclave_path);
	    return -1;
	}

	if (pet_ioctl_fd(ctrl_fd, ENCLAVE_CMD_FREE_V3_PCI,  &dev_spec) != 0) {
	    printf("Error: Could not remove device from Palacios\n");
	    return -1;
	}


	if (pet_online_pci(bus, dev, fn) != 0) {
	    printf("Error: Could not online PCI device (%s)\n", bdf_str);
	    return -1;
	}
	
    } else {
	usage();
	return -1;
    }

    return 0;
}
