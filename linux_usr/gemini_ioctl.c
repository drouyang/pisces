#include"../inc/gemini_dev.h"

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
            printf("Usage: ./gemini_ioctl <num>\n");
            printf("1000 PING\n");
            printf("1001 PREPARE_SECONDARY\n");
            printf("1002 LOAD_IMAGE\n");
            printf("1003 START_SECONDARY\n");
            printf("1005 READ_CONSOLE_BUFFER\n");
            exit(-1);
        }

        ioctl_num = atoi(argv[1]);
        if (ioctl_num < G_IOCTL_MIN || ioctl_num > G_IOCTL_MAX) {
            printf("Error: ioctl number %d out of range.\n", ioctl_num);
            exit(-1);
        }

        file_desc = open("/dev/" DEVICE_NAME, 0);
        if (file_desc < 0) {
                printf("Can't open device file: %s\n", "/dev/"DEVICE_NAME);
                exit(-1);
        }

        ret_val = ioctl(file_desc, ioctl_num, NULL);

        if (ret_val < 0) {
                printf("ioctl %d failed on error %d\n", ioctl_num, ret_val);
                exit(-1);
        }

        close(file_desc);
}
