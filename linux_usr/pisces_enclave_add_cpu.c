#include"../src/pisces.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>              /* open */
#include <unistd.h>             /* exit */
#include <sys/ioctl.h>          /* ioctl */


uint64_t ioctl_num = 0;

int main(int argc, char *argv[])
{
        int file_desc, ret_val;
        char dev[80] = "/dev/";

        if (argc < 3) {
            printf("Usage: ./pisces_add_cpu <device_file_path> <apicid>\n");
            exit(-1);
        }

        strcpy(dev, argv[1]);
        ioctl_num = atoi(argv[2]);

        //printf("<%s, %d>\n", dev, ioctl_num);

        file_desc = open(dev, O_RDONLY);
        if (file_desc < 0) {
                printf("Can't open device file: %s\n", dev);
                exit(-1);
        }

        ret_val = ioctl(file_desc, PISCES_ENCLAVE_ADD_CPU, &ioctl_num);

        if (ret_val < 0) {
                printf("ioctl %d failed on error %d\n", ioctl_num, ret_val);
                exit(-1);
        }

        close(file_desc);
}
