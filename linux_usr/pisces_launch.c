#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <sys/ioctl.h>

#include "../src/pisces.h"

int main(int argc, char ** argv) {
    int pisces_fd = 0;

    char * enclave_path = argv[1];

    if (argc <= 1) {
        printf("Usage: %s <enclave_dev_path>\n", argv[0]);
        return -1;
    }

    pisces_fd = open(enclave_path, O_RDONLY);

    if (pisces_fd == -1) {
        printf("Error opening Enclave device\n");
        return -1;
    }

    ioctl(pisces_fd, PISCES_ENCLAVE_LAUNCH, NULL);
    /* Close the file descriptor.  */
    close(pisces_fd);
}
