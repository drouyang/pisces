/*
 * Pisces Enclave control Utility
 * (c) Jiannan Ouyang, 2013
 */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> 
#include <fcntl.h>
#include <sys/stat.h>

#include "../src/pisces.h"


int main(int argc, char* argv[]) {
    int enclave_fd;
    int ctrl_fd;
    char * enclave_dev = NULL;

    if (argc < 2) {
      printf("usage: pisces_ctrl <enclave_device> <cmd>\n");
      return -1;
    }

    enclave_dev = argv[1];

    enclave_fd = open(enclave_dev, O_RDONLY);

    if (enclave_fd == -1) {
      printf("Error opening enclave device: %s\n", enclave_dev);
      return -1;
    }

    ctrl_fd = ioctl(enclave_fd, PISCES_ENCLAVE_CTRL_CONNECT, NULL);

    close(enclave_fd);

    if (ctrl_fd < 0) {
      printf("Error opening enclave console\n");
      return -1;
    }

    ioctl(ctrl_fd, 101, NULL);

    close(ctrl_fd);

    return 0;
}
