#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <sys/ioctl.h> 

#include "../src/pisces_dev.h"

int main(int argc, char ** argv) {
 
    int pisces_fd = 0;



    pisces_fd = open("/dev/" DEVICE_NAME, O_RDONLY);

    if (pisces_fd == -1) {
        printf("Error opening fastmem control device\n");
        return -1;
    }

    ioctl(pisces_fd, P_IOCTL_LAUNCH_ENCLAVE, NULL); 
    /* Close the file descriptor.  */ 
    close(pisces_fd);
    


}
