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
 
#include <pet_types.h>
#include <pet_ioctl.h>

#include "../src/pisces.h"
#include "../src/ctrl_cmds.h"

int main(int argc, char* argv[]) {
    char * enclave_path = argv[1];
    int ctrl_fd;
    int err;

    if (argc <= 2) {
	printf("usage: v3_dbg <pisces_device>\n");
	return -1;
    }

    printf("Debug VM (%s)\n", enclave_path);
    
 
    ctrl_fd = pet_ioctl_path(enclave_path, PISCES_ENCLAVE_CTRL_CONNECT, NULL);

    if (ctrl_fd < 0) {
        printf("Error opening enclave control channel (%s)\n", enclave_path);
        return -1;
    }

    if (pet_ioctl_fd(ctrl_fd, ENCLAVE_CMD_VM_DBG, NULL) != 0) {
	printf("Error: Could not LAUNCH VM\n");
	return -1;
    }


    return 0; 
} 


