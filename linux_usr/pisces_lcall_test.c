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


int main(int argc, char* argv[]) {
    char * enclave_path = NULL;
    int ctrl_fd = 0;
    int lcall_fd = 0;



	enclave_path = argv[1];


    ctrl_fd = pet_ioctl_path(enclave_path, PISCES_ENCLAVE_CTRL_CONNECT, NULL);

    if (ctrl_fd < 0) {
	printf("Error opening enclave control channel (%s)\n", enclave_path);
	return -1;
    }

    lcall_fd = pet_ioctl_path(enclave_path, PISCES_ENCLAVE_LCALL_CONNECT, NULL);

  


    if (pet_ioctl_fd(ctrl_fd, ENCLAVE_IOCTL_TEST_LCALL, (void *)NULL) != 0) {
	printf("Error: Could not test lcall\n");
	return -1;
    }
	

    close(ctrl_fd);

    return 0;
}
