#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <regex.h>
#include <errno.h>
#include <fcntl.h>

#include <pet_ioctl.h>

#include "pisces.h"



static void usage() {
    printf("Usage: pisces_reboot <enclave_path>\n");
    exit(-1);
}




int main(int argc, char ** argv) {
    char * enclave_path  = argv[1];

    if (argc < 2) {
	usage();
	return -1;
    }

    printf("Sending shutdown command to enclave (%s)\n", enclave_path);

    /* Send reset request to Enclave */
    pisces_reset(get_pisces_id_from_path(enclave_path));

    return 0;
}
