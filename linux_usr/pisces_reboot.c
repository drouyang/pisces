#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <regex.h>
#include <errno.h>
#include <fcntl.h>

#include <pet_ioctl.h>
#include <pet_mem.h>
#include <pet_cpu.h>

#include "list.h"

#include "../src/pisces.h"
#include "../src/ctrl_cmds.h"

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


static struct list_head memory_list;
static struct list_head cpu_list;
static struct list_head pci_list;


static void usage() {
    printf("Usage: pisces_reboot <enclave_id> <kernel binary> <initrd binary> <kernel cmd line>\n");
    exit(-1);
}




int main(int argc, char ** argv) {
    char enclave_path[128]  = {[0 ... 127] = 0};
    int  enclave_id         = -1;

    char * kern     = NULL;
    char * initrd   = NULL;
    char * cmd_line = NULL;

    struct enclave_boot_env boot_env;
    struct pisces_image img;

    memset(&boot_env, 0, sizeof(struct enclave_boot_env));
    memset(&img, 0, sizeof(struct pisces_image));

    if (argc != 5) {
	usage();
    }

    enclave_id = atoi(argv[1]);
    kern       = argv[2];
    initrd     = argv[3];
    cmd_line   = argv[4];

    /* Setup the image */
    {
	img.enclave_id = enclave_id;
	img.kern_fd    = open(kern, O_RDONLY);
	if (img.kern_fd == -1) {
	    printf("Error: Could not find valid kernel image at (%s)\n", kern);
	    return -1;
	}

	img.init_fd    = open(initrd, O_RDONLY);
	if (img.init_fd == -1) {
	    printf("Error: Could not find valid initrd file at (%s)\n", kern);
	    return -1;
	}

	strncpy(img.cmd_line, cmd_line, 1024);
    }

    snprintf(enclave_path, 127, DEV_PATH "%d", enclave_id);

    printf("Rebooting Enclave (%s)\n", enclave_path);

    /* Cache Resources in use */
    INIT_LIST_HEAD(&(cpu_list));
    INIT_LIST_HEAD(&(memory_list));
    INIT_LIST_HEAD(&(pci_list));

    printf("Checking for assigned CPUs\n");

    /* CPUs */
    {
	char   cpu_path[128]   = {[0 ... 127] = 0};
	char   hdr_str[64]     = {[0 ... 63] = 0};
	char   cpu_str[32]     = {[0 ... 31] = 0};
	FILE * cpu_file        = NULL;
	unsigned long num_cpus = 0;

	int i = 0;
	

	snprintf(cpu_path, 127, PROC_PATH "enclave-%d/cpus", enclave_id);

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

	    list_add(&(res->node), &cpu_list);
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
	

	snprintf(mem_path, 127, PROC_PATH "enclave-%d/memory", enclave_id);

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

	    list_add_tail(&(res->node), &memory_list);
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

	snprintf(pci_path, 127, PROC_PATH "enclave-%d/pci", enclave_id);

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

	    list_add(&(res->node), &pci_list);
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

    /* Relaunch enclave on contiguous memory region and 1 CPU */
    {
	struct enclave_resource * res = NULL;
	struct enclave_resource * next = NULL;

	boot_env.block_size = pet_block_size();

	unsigned long long prev_end   = 0;
	int                num_blocks = 0;

	list_for_each_entry_safe(res, next, &memory_list, node) {
	    if (boot_env.num_blocks == 0) {
		boot_env.base_addr = res->mem_res.start_addr;
	    } else if (res->mem_res.start_addr != prev_end) {
		/* Discontiguous - finish the boot env */
		break;
	    }

	    prev_end = res->mem_res.end_addr;
	    boot_env.num_blocks++;
	    
	    list_del(&res->node);
	}

	/* Grab a single CPU */
	res = list_first_entry(&cpu_list, struct enclave_resource, node);

	boot_env.cpu_id = res->cpu_res.cpu_id;

        /* Online it first, then offline again */
	pet_online_cpu(boot_env.cpu_id);

	pet_offline_cpu(boot_env.cpu_id);

	list_del(&res->node);
    }

    printf("Onlining Remaining Resources\n");

    /* Online all resources */
    {
	struct enclave_resource * res = NULL;
	struct enclave_resource * next = NULL;

	list_for_each_entry_safe(res, next, &cpu_list, node) {
	    printf("Onlining CPU %d\n", res->cpu_res.cpu_id);

	    pet_online_cpu(res->cpu_res.cpu_id);

	    list_del(&res->node);
	}

	list_for_each_entry_safe(res, next, &memory_list, node) {
	    int num_blks = (res->mem_res.end_addr - res->mem_res.start_addr) / pet_block_size();
	    int i        = 0;
	    
	    for (i = 0; i < num_blks; i++) {
		int blk_id = (res->mem_res.start_addr / pet_block_size()) + i;

		printf("Onlining MEM block: %d\n", blk_id);

		pet_online_block(blk_id);
	    }

	    list_del(&res->node);
	}

	list_for_each_entry_safe(res, next, &pci_list, node) {
	    unsigned int bus = 0;
	    unsigned int dev = 0;
	    unsigned int fn  = 0;
	    
	    if (pet_parse_bdf(res->pci_res.bdf, &bus, &dev, &fn) != 0) {
		printf("Error: Could not parse BDF spcification string (%s)\n", res->pci_res.bdf);
		return -1;
	    }

	    pet_online_pci(bus, dev, fn);

	    list_del(&res->node);
	}
    }


    printf("Rebooting enclave on memory range [%p, %p) and CPU %d\n",
	    (void *)boot_env.base_addr,
	    (void *)(boot_env.base_addr + (boot_env.num_blocks * boot_env.block_size)),
	    boot_env.cpu_id);

    {
	/* Issue a reset command to get a new enclave id */
	if (pet_ioctl_path(PISCES_PATH, PISCES_RELOAD_IMAGE, (void *)&img) < 0) {
	    printf("Error: Could not reload enclave %d image\n", enclave_id);
	    return -1;
	}


	/* Launch it */
	{
	    char path[128];
	    int  new_id = img.enclave_id;
	    int  ret    = 0;

	    snprintf(path, 128, "/dev/pisces-enclave%d", new_id);

	    /* Issue a re-launch */
	    ret = pet_ioctl_path(path, PISCES_ENCLAVE_LAUNCH, &boot_env);

	    if (ret < 0) {
		printf("Error launching enclave. code=%d\n", ret);
	    } else {
		printf("Rebooted enclave with id %d\n", new_id);
	    }
	}
    }

    return 0;
}
