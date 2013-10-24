#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>



#include "../src/pisces.h"
#include <pet_ioctl.h>

int main(int argc, char ** argv) {
    char * kern = argv[1];
    char * initrd = argv[2];
    char * cmd_line = argv[3];

    struct pisces_image * img = NULL;

    img = malloc(sizeof(struct pisces_image));

    strcpy(img->kern_path, kern);
    strcpy(img->initrd_path, initrd);
    strcpy(img->cmd_line, cmd_line);

    pet_ioctl_path("/dev/" DEVICE_NAME, PISCES_LOAD_IMAGE, img);
}
