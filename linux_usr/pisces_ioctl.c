#include"../src/pisces.h"

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>              /* open */
#include <unistd.h>             /* exit */
#include <sys/ioctl.h>          /* ioctl */


int main(int argc, char *argv[])
{
        int file_desc, ret_val;
        int ioctl_num = 0;

        if (argc < 2) {
            printf("Usage: ./pisces_ioctl <num>\n");
            printf("1000 PING\n");
            printf("1001 PREPARE_SECONDARY\n");
            printf("1002 LOAD_IMAGE\n");
            printf("1003 START_SECONDARY\n");
            printf("1005 EXIT\n");
            exit(-1);
        }

        ioctl_num = atoi(argv[1]);

        file_desc = open("/dev/pisces-enclave0", 0);
        if (file_desc < 0) {
                printf("Can't open device file: %s\n", "/dev/pisces-enclave0");
                exit(-1);
        }

        ret_val = ioctl(file_desc, ioctl_num, NULL);

        if (ret_val < 0) {
                printf("ioctl %d failed on error %d\n", ioctl_num, ret_val);
                exit(-1);
        }

        close(file_desc);
}
