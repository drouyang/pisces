#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <linux/types.h>
#include "../src/pisces.h"

void dump_cmd(struct pisces_ctrl_cmd * cmd)
{
    printf("cmd: (%llu, (%llu, %llu, %llu))\n", 
            (unsigned long long) cmd->cmd, 
            (unsigned long long) cmd->arg1, 
            (unsigned long long) cmd->arg2, 
            (unsigned long long) cmd->arg3);
}

char device_name[100] = "pisces-enclave0";

int
main(int argc, char *argv[], char *envp[])
{
    int pisces_fd = 0;
    struct pisces_ctrl_cmd cmd;

    pisces_fd = open("/dev/pisces-enclave0", O_RDWR);
    if (pisces_fd == -1) {
        printf("Error opening /dev/pisces-enclave0\n");
        return -1;
    }

#if 0
    /* send a command */
    cmd.cmd = 5;
    cmd.arg1 = 6;
    cmd.arg2 = 7;
    cmd.arg3 = 8;
    if (write(pisces_fd, &cmd, sizeof(cmd)) == -1) {
        printf("Error writing /dev/pisces-enclave0\n");
        return -1;
    }
#endif

    while(1) {
        /* receive a command */
        if (read(pisces_fd, &cmd, sizeof(cmd)) == -1) {
            printf("Error reading /dev/pisces-enclave0\n");
            return -1;
        }
        switch (cmd.cmd) {
            case PISCES_ENCLAVE_BOOT_CPU:
                {
                    int ret;

                    printf("Command received %llu\n", cmd.cmd);
                    ret = ioctl(pisces_fd, PISCES_ENCLAVE_BOOT_CPU, &cmd);
                    if (ret < 0)
                        printf("Pisces: ioctl %d failed.\n", cmd.cmd);
                    break;
                }

            default:
                printf("Pisces: unknown command received.\n");
                dump_cmd(&cmd);
        }
    }

    close(pisces_fd);
    return 0;
}

