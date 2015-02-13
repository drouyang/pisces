#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <regex.h>
#include <errno.h>
#include <stdint.h>


#include "list.h"

#include "pisces_ioctl.h"
#include "ctrl_ioctl.h"

#include <pet_ioctl.h>
#include <pet_mem.h>
#include <pet_cpu.h>

#define PISCES_PATH "/dev/pisces"
#define DEV_PATH    "/dev/pisces-enclave"
#define PROC_PATH   "/proc/pisces/"

#define CPU_HDR_STR       "Num CPUs: "
#define CPU_ENTRY_STR     "CPU "
#define MEM_HDR_STR       "Num Memory Blocks: "
#define MEM_REGEX_STR     "[0-9]+: ([0-9A-Fa-f]{16}) - ([0-9A-Fa-f]{16})"
#define PCI_HDR_STR       "PCI Devices: "
#define PCI_REGEX_STR     "(\\s*): ([0-9]{4}?:{0,1}[0-9A-Fa-f]{2}:[0-9A-Fa-f]{2}.[0-7])"


typedef enum { INVALID_RESOURCE, 
	       CPU_RESOURCE,
	       MEM_RESOURCE,
	       PCI_RESOURCE } res_type_t;

struct cpu_res_info {
    unsigned int cpu_id;
};


struct mem_res_info {
    unsigned long long start_addr;
    unsigned long long end_addr;
};

struct pci_res_info {
    char name[128];
    char bdf[128];
};


struct enclave_resource {
    res_type_t type;

    union {
	struct cpu_res_info cpu_res;
	struct mem_res_info mem_res;
	struct pci_res_info pci_res;
    };

    struct list_head node;
};


static struct list_head resource_list;




int
pisces_teardown(int pisces_id) 
{
    char enclave_path[128]  = {[0 ... 127] = 0};



    snprintf(enclave_path, 127, DEV_PATH "%d", pisces_id);

    printf("Shutting down Enclave (%s)\n", enclave_path);

    /* Cache Resources in use */
    INIT_LIST_HEAD(&(resource_list));


    printf("Checking for assigned CPUs\n");

    /* CPUs */
    {
	char   cpu_path[128]   = {[0 ... 127] = 0};
	char   hdr_str[64]     = {[0 ... 63] = 0};
	char   cpu_str[32]     = {[0 ... 31] = 0};
	FILE * cpu_file        = NULL;
	unsigned long num_cpus = 0;

	int i = 0;
	

	snprintf(cpu_path, 127, PROC_PATH "enclave-%d/cpus", pisces_id);

	cpu_file = fopen(cpu_path, "r");

	if (cpu_file == NULL) {
	    printf("Error: Could not open CPU proc file\n");
	    return -1;
	}

	if (fgets(hdr_str, 63, cpu_file) == NULL) {
	    printf("Error: Could not read CPU proc file\n");
	    return -1;
	}
	
	if (strncmp(hdr_str, CPU_HDR_STR, strlen(CPU_HDR_STR)) != 0) {
	    printf("Error: Invalid CPU proc file header (%s)\n", hdr_str);
	    return -1;
	}

	num_cpus = atoi(hdr_str + strlen(CPU_HDR_STR));

	for (i = 0; i < num_cpus; i++) {
	    struct enclave_resource * res = NULL; 


	    if (fgets(cpu_str, 31, cpu_file) == NULL) {
		printf("Error: Could not read CPU entry\n");
		return -1;
	    }

	    if (strncmp(cpu_str, CPU_ENTRY_STR, strlen(CPU_ENTRY_STR)) != 0) {
		printf("Error: Invalid CPU Entry (%s)\n", cpu_str);
		return -1;
	    }


	    res = (struct enclave_resource *)malloc(sizeof(struct enclave_resource));
	    
	    memset(res, 0, sizeof(struct enclave_resource));

	    res->type           = CPU_RESOURCE;
	    res->cpu_res.cpu_id = atoi(cpu_str + strlen(CPU_ENTRY_STR));

	    list_add(&(res->node), &resource_list);
	}
    }

    printf("Checking for memory blocks\n");

    /* Memory */
    {

	char   mem_path[128]   = {[0 ... 127] = 0};
	char   hdr_str[64]     = {[0 ... 63] = 0};
	char   mem_str[128]    = {[0 ... 127] = 0};
	FILE * mem_file        = NULL;
	unsigned long num_blks = 0;

	int i = 0;
	

	snprintf(mem_path, 127, PROC_PATH "enclave-%d/memory", pisces_id);

	mem_file = fopen(mem_path, "r");

	if (mem_file == NULL) {
	    printf("Error: Could not open MEM proc file\n");
	    return -1;
	}

	if (fgets(hdr_str, 63, mem_file) == NULL) {
	    printf("Error: Could not read MEM proc file\n");
	    return -1;
	}
	
	if (strncmp(hdr_str, MEM_HDR_STR, strlen(MEM_HDR_STR)) != 0) {
	    printf("Error: Invalid MEM proc file header (%s)\n", hdr_str);
	    return -1;
	}

	num_blks = atoi(hdr_str + strlen(MEM_HDR_STR));

	for (i = 0; i < num_blks; i++) {
	    struct enclave_resource * res = NULL; 

	    char       start_str[17] = {[0 ... 16] = 0};
	    char       end_str[17]   = {[0 ... 16] = 0};
	    regmatch_t matches[3];
	    regex_t    regex;

	    if (regcomp(&regex, MEM_REGEX_STR, REG_EXTENDED) != 0) {
		printf("Error: Compiling Regular Expression (%s)\n", MEM_REGEX_STR);
		return -1;
	    }


	    if (fgets(mem_str, 127, mem_file) == NULL) {
		printf("Error: Could not read MEM entry\n");
		return -1;
	    }


	    if (regexec(&regex, mem_str, 3, matches, 0) != 0) {
		printf("Error: Invalid Mem entry string  (%s)\n", mem_str);
		return -1;
	    }

	    if ( ((matches[1].rm_eo - matches[1].rm_so) != 16) ||
		 ((matches[2].rm_eo - matches[2].rm_so) != 16) ) {
		printf("Error: Invalid mem entry string (%s)\n", mem_str);
	    }
	    
	    memcpy(start_str, mem_str + matches[1].rm_so, 16);
	    memcpy(end_str,   mem_str + matches[2].rm_so, 16);
	    
	    res = (struct enclave_resource *)malloc(sizeof(struct enclave_resource));
	    
	    memset(res, 0, sizeof(struct enclave_resource));

	    res->type               = MEM_RESOURCE;
	    res->mem_res.start_addr = strtol(start_str, NULL, 16);
	    res->mem_res.end_addr   = strtol(end_str,   NULL, 16);


	    list_add(&(res->node), &resource_list);
	}
    }

    printf("Checking for PCI Devices\n");
    
    /* PCI Devices */
    {
	char   pci_path[128]   = {[0 ... 127] = 0};
	char   hdr_str[64]     = {[0 ... 63] = 0};
	char   pci_str[256]    = {[0 ... 255] = 0};
	FILE * pci_file        = NULL;
	unsigned long num_devs = 0;

	int i = 0;

	snprintf(pci_path, 127, PROC_PATH "enclave-%d/pci", pisces_id);

	pci_file = fopen(pci_path, "r");

	if (pci_file == NULL) {
	    printf("Error: Could not open PCI proc file\n");
	    return -1;
	}

	if (fgets(hdr_str, 63, pci_file) == NULL) {
	    printf("Error: Could not read PCI proc file\n");
	    return -1;
	}
	
	if (strncmp(hdr_str, PCI_HDR_STR, strlen(PCI_HDR_STR)) != 0) {
	    printf("Error: Invalid PCI proc file header (%s)\n", hdr_str);
	    return -1;
	}

	num_devs = atoi(hdr_str + strlen(PCI_HDR_STR));

	for (i = 0; i < num_devs; i++) {
	    struct enclave_resource * res = NULL; 

	    char name_str[128] = {[0 ... 127] = 0};
	    char bdf_str[128]  = {[0 ... 127] = 0};

	    regmatch_t matches[3];
	    regex_t    regex;
	

	    if (regcomp(&regex, PCI_REGEX_STR, REG_EXTENDED) != 0) {
		printf("Error: Compiling Regular Expression (%s)\n", PCI_REGEX_STR);
		return -1;
	    }


	    if (fgets(pci_str, 255, pci_file) == NULL) {
		printf("Error: Could not read PCI entry\n");
		return -1;
	    }

	    if (regexec(&regex, pci_str, 3, matches, 0) != 0) {
		printf("Error: Invalid PCI entry string  (%s)\n", pci_str);
		return -1;
	    }

	    if ( ((matches[1].rm_eo - matches[1].rm_so) >= 128) ||
		 ((matches[2].rm_eo - matches[2].rm_so) >= 128) ) {
		printf("Error: Invalid mem entry string (%s)\n", pci_str);
	    }
	    
	    memcpy(name_str, pci_str + matches[1].rm_so, (matches[1].rm_eo - matches[1].rm_so) );
	    memcpy(bdf_str,  pci_str + matches[2].rm_so, (matches[2].rm_eo - matches[2].rm_so) );
	    

	    res = (struct enclave_resource *)malloc(sizeof(struct enclave_resource));
	    
	    memset(res, 0, sizeof(struct enclave_resource));

	    res->type        = PCI_RESOURCE;
	    strncpy(res->pci_res.name, name_str, strlen(name_str));
	    strncpy(res->pci_res.bdf,  bdf_str,  strlen(bdf_str));

	    list_add(&(res->node), &resource_list);
	}


    }


    printf("Sending shutdown command to enclave\n");

    /* Send shutdown request to Enclave */
    {
	int ctrl_fd = 0;

	ctrl_fd = pet_ioctl_path(enclave_path, PISCES_ENCLAVE_CTRL_CONNECT, NULL);

	if (ctrl_fd < 0) {
	    printf("Error opening enclave control channel (%s)\n", enclave_path);
	    return -1;
	}
	
	if (pet_ioctl_fd(ctrl_fd, ENCLAVE_CMD_SHUTDOWN, 0) != 0) {
	    printf("Error: Could not LAUNCH VM\n");
	    return -1;
	}

	close(ctrl_fd);
    }


    printf("Freeing enclave\n");

    /* Free Enclave */
    {
	pet_ioctl_path(PISCES_PATH, PISCES_FREE_ENCLAVE, (void *)(uint64_t)pisces_id);
    }

    
    printf("Onlining Resources\n");

    /* Online all resources */
    {

	struct enclave_resource * res = NULL;

	list_for_each_entry(res, &resource_list, node) {
	    if (res->type == CPU_RESOURCE) {

		printf("Onlining CPU %d\n", res->cpu_res.cpu_id);

		pet_online_cpu(res->cpu_res.cpu_id);

	    } else if (res->type == MEM_RESOURCE) {
		int num_blks = (res->mem_res.end_addr - res->mem_res.start_addr) / pet_block_size();
		int i        = 0;
		
		for (i = 0; i < num_blks; i++) {
		    int blk_id = (res->mem_res.start_addr / pet_block_size()) + i;

		    printf("Onlining MEM block: %d\n", blk_id);

		    pet_online_block(blk_id);
		}
	    } else if (res->type == PCI_RESOURCE) {
		unsigned int bus = 0;
		unsigned int dev = 0;
		unsigned int fn  = 0;
		
		if (pet_parse_bdf(res->pci_res.bdf, &bus, &dev, &fn) != 0) {
		    printf("Error: Could not parse BDF spcification string (%s)\n", res->pci_res.bdf);
		    return -1;
		}

		pet_online_pci(bus, dev, fn);

	    } else {
		printf("Invalid resource type\n");
	    }
	}
    }



    return 0;
}
