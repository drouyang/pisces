#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>



#include "../src/pisces.h"
#include <pet_ioctl.h>

int main(int argc, char ** argv) {
    char * kern     = argv[1];
    char * initrd   = argv[2];
    char * cmd_line = argv[3];
    int    kern_fd  = 0;
    int    init_fd  = 0;

    struct pisces_image * img = NULL;
    int    enclave_id = 0;


    kern_fd = open(kern, O_RDONLY);
    if (kern_fd == -1) {
	printf("Error: Could not find valid kernel image at (%s)\n", kern);
	return -1;
    }

    init_fd = open(initrd, O_RDONLY);
    if (init_fd == -1) {
	printf("Error: Could not find valid initrd file at (%s)\n", initrd);
	return -1;
    }

    img = malloc(sizeof(struct pisces_image));

    img->kern_fd = kern_fd;
    img->init_fd = init_fd;
    strcpy(img->cmd_line, cmd_line);

    enclave_id = pet_ioctl_path("/dev/" DEVICE_NAME, PISCES_LOAD_IMAGE, img);
    
    if (enclave_id < 0 ) {
	printf("Error: Could not create enclave (Error Code: %d)\n", enclave_id);
	return -1;
    }

    printf("Enclave Created: /dev/pisces-enclave%d\n", enclave_id);

    return enclave_id;
}
