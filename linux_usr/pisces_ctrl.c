/*
 * Pisces Enclave control Utility
 * (c) Jiannan Ouyang, 2013
 */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> 
#include <fcntl.h>
#include <sys/stat.h>


#include <pet_mem.h>
#include <pet_ioctl.h>
#include <pet_cpu.h>

#include "../src/pisces.h"
#include "../src/pisces_cmds.h"



int main(int argc, char* argv[]) {
    int ctrl_fd;
    char * enclave_dev = NULL;

    if (argc < 2) {
      printf("usage: pisces_ctrl <enclave_device> <cmd>\n");
      return -1;
    }

    ctrl_fd = pet_ioctl_path(enclave_dev, PISCES_ENCLAVE_CTRL_CONNECT, NULL);

    if (ctrl_fd < 0) {
      printf("Error opening enclave console\n");
      return -1;
    }

    pet_ioctl_fd(ctrl_fd, 101, NULL);

    close(ctrl_fd);

    return 0;
}
