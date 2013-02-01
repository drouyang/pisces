#include"../inc/gemini_dev.h"

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>              /* open */
#include <unistd.h>             /* exit */
#include <sys/ioctl.h>          /* ioctl */


void ioctl_ping(int file_desc)
{
        int ret_val;

        ret_val = ioctl(file_desc, G_IOCTL_PING, NULL);

        if (ret_val < 0) {
                printf("ioctl_ping failed:%d\n", ret_val);
                exit(-1);
        }
}

main()
{
        int file_desc, ret_val;
        char *msg = "Message passed by ioctl\n";

        file_desc = open("/dev/" DEVICE_NAME, 0);
        if (file_desc < 0) {
                printf("Can't open device file: %s\n", "/dev/"DEVICE_NAME);
                exit(-1);
        }

        ioctl_ping(file_desc);

        close(file_desc);
}
