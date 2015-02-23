#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <getopt.h>



#include "pisces_ctrl.h"
#include "pisces.h"



static void usage() {
    printf("pisces_load_file <enclave_dev> <local file> <remote file>\n");
    
    exit(-1);
}


int main(int argc, char ** argv) {
    int    pisces_id   = -1;
    int    ctrl_fd     =  0;
    char * local_file  = NULL;
    char * remote_file = NULL;

    if (argc < 3) {
	usage();
    }

    pisces_id   = get_pisces_id_from_path(argv[1]);
    local_file  = argv[2];
    remote_file = argv[3];

    return pisces_load_file(pisces_id, local_file, remote_file);
}
