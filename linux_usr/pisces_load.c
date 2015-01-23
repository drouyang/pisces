#include <stdio.h>

#include "pisces.h"


int main(int argc, char ** argv) {
    char * kern       = argv[1];
    char * initrd     = argv[2];
    char * cmd_line   = argv[3];
    int    enclave_id = 0;
    

    enclave_id = pisces_load(kern, initrd, cmd_line);

    printf("Enclave Created: %d\n", enclave_id);

    return enclave_id;
}
