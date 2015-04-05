/* 
 * Pisces/V3 Control utility
 * (c) Jack lange, 2013
 */


#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h> 
#include <sys/ioctl.h> 
#include <sys/stat.h> 
#include <sys/types.h> 
#include <unistd.h> 
#include <string.h>
#include <stdint.h>

#include <pet_ioctl.h>

#include "pisces_ioctl.h"
#include "ctrl_ioctl.h"



#define PRINT_TELEMETRY  0x00000001
#define PRINT_CORE_STATE 0x00000002
#define PRINT_ARCH_STATE 0x00000004
#define PRINT_STACK      0x00000008
#define PRINT_BACKTRACE  0x00000010

#define CLEAR_COUNTERS   0x40000000
#define SINGLE_EXIT_MODE 0x80000000 // begin single exit when this flag is set, until flag is cleared.


void usage() {

	printf("usage: v3_debug <flags> <enclave_device> <vm_id> [vm core]\n");
	printf("\t-t: Print telemetry\n");
	printf("\t-c: Print Core State\n");
	printf("\t-a: Print Architecture State\n");
	printf("\t-s: Print Stack Trace\n");
	printf("\t-b: Print Backtrace\n");
	printf("\n");
	printf("\t-C: Clear counters\n");
	printf("\t-S: Enable single exit mode\n");
	return;

}


int main(int argc, char* argv[]) {
    struct pisces_dbg_spec cmd;
    char * enclave_path   = NULL;

    int ctrl_fd           = 0;
    int num_opts          = 0;
    int c                 = 0;

    memset(&cmd, 0, sizeof(struct pisces_dbg_spec));
    
    opterr = 0;

    while ((c = getopt(argc, argv, "tcasbS")) != -1) {
	num_opts++;

	switch (c) {
	    case 't': 
		cmd.cmd |= PRINT_TELEMETRY;
		break;
	    case 'c': 
		cmd.cmd |= PRINT_CORE_STATE;
		break;
	    case 'a': 
		cmd.cmd |= PRINT_ARCH_STATE;
		break;
	    case 's':
		cmd.cmd |= PRINT_STACK;
		break;
	    case 'b':
		cmd.cmd |= PRINT_BACKTRACE;
		break;
	    case 'C':
		cmd.cmd |= CLEAR_COUNTERS;
		break;
	    case 'S':
		cmd.cmd |= SINGLE_EXIT_MODE;
		break;
	}
    }

    if (argc - optind + num_opts < 3) {
	usage();
	return -1;
    }

    enclave_path  = argv[optind];
    cmd.vm_id     = atoi(argv[optind + 1]);

    if (argv[optind + 2] == NULL) {
	cmd.core  = -1;
    } else {
	cmd.core  = atoi(argv[optind + 2]); // No, the reversed argument ordering doesn't make sense...
    }

    printf("Sending Debug command %x to VM %d (vcpu: %d) on enclave %s\n", 
	    cmd.cmd, cmd.vm_id, cmd.core,  enclave_path);

 
    ctrl_fd = pet_ioctl_path(enclave_path, PISCES_ENCLAVE_CTRL_CONNECT, NULL);

    if (ctrl_fd < 0) {
        printf("Error opening enclave control channel (%s)\n", enclave_path);
        return -1;
    }

    if (pet_ioctl_fd(ctrl_fd, ENCLAVE_CMD_VM_DBG, &cmd) != 0) {
	printf("Error: Could not send debug request to VM\n");
	return -1;
    }

    return 0; 
} 


