#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <regex.h>
#include <errno.h>


#include "pisces.h"

static void usage() {
    printf("Usage: pisces_shutdown <enclave_id>\n");
    exit(-1);
}




int main(int argc, char ** argv) {
    int  enclave_id         = -1;

    if (argc < 2) {
	usage();
    }


    enclave_id = atoi(argv[1]);

    printf("Shutting down Enclave (%d)\n", enclave_id);


    pisces_teardown(enclave_id);
    return 0;
}
