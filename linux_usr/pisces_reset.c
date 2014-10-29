#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <regex.h>
#include <errno.h>
#include <fcntl.h>

#include <pet_ioctl.h>

#include "../src/pisces.h"



static void usage() {
    printf("Usage: pisces_reboot <enclave_path>\n");
    exit(-1);
}




int main(int argc, char ** argv) {
    char * enclave_path  = argv[1];

    printf("Sending shutdown command to enclave (%s)\n", enclave_path);

    /* Send shutdown request to Enclave */

    pet_ioctl_path(enclave_path, PISCES_ENCLAVE_RESET, NULL);
	

    return 0;
}
