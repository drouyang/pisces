#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>              /* open */
#include <unistd.h>             /* exit */
#include <sys/ioctl.h>          /* ioctl */


typedef signed char s8;
typedef unsigned char u8;

typedef signed short s16;
typedef unsigned short u16;

typedef signed int s32;
typedef unsigned int u32;

typedef signed long long s64;
typedef unsigned long long u64;

#include"../src/pisces.h"
#include"../src/pisces_cmds.h"


uint64_t apicid = 0;

int main(int argc, char *argv[])
{
    int enclave_fd;
    int ctrl_fd;
    int ret_val;
    char * enclave_dev = NULL;

    if (argc < 3) {
        printf("Usage: ./pisces_add_cpu <device_file_path> <apicid>\n");
        exit(-1);
    }

    enclave_dev = argv[1];
    enclave_fd = open(enclave_dev, O_RDONLY);
    if (enclave_fd == -1) {
      printf("Error opening enclave device: %s\n", enclave_dev);
      return -1;
    }

    apicid = atoi(argv[2]);

    ctrl_fd = ioctl(enclave_fd, PISCES_ENCLAVE_CTRL_CONNECT, NULL);

    close(enclave_fd);

    if (ctrl_fd < 0) {
      printf("Error opening enclave ctrl\n");
      return -1;
    }

    ret_val = ioctl(ctrl_fd, ENCLAVE_CMD_ADD_CPU, &apicid);

    if (ret_val < 0) {
        printf("ioctl %d failed on error %d\n", apicid, ret_val);
        exit(-1);
    }

    close(ctrl_fd);
}
