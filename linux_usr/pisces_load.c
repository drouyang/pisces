#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <sys/ioctl.h>

#include "../src/pisces.h"

int main(int argc, char ** argv) {
    char * kern = argv[1];
    char * initrd = argv[2];
    char * cmd_line = argv[3];

    int pisces_fd = 0;
    struct pisces_image * img = NULL;


    img = malloc(sizeof(struct pisces_image));


    strcpy(img->kern_path, kern);
    strcpy(img->initrd_path, initrd);
    strcpy(img->cmd_line, cmd_line);


    pisces_fd = open("/dev/" DEVICE_NAME, O_RDONLY);

    if (pisces_fd == -1) {
        printf("Error opening fastmem control device\n");
        return -1;
    }

    ioctl(pisces_fd, PISCES_LOAD_IMAGE, img);
    /* Close the file descriptor.  */
    close(pisces_fd);
}
