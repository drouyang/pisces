/*
 * Pisces Enclave Console Utility
 * (c) Jiannan Ouyang, 2013
 */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "../src/pisces.h"
#define INBUF_SIZE 4096

int main(int argc, char* argv[]) {
    int enclave_fd;
    int cons_fd;
    char * enclave_dev = NULL;
    char inbuf[INBUF_SIZE + 1];

    if (argc < 2) {
      printf("usage: pisces_cons <enclave_device>\n");
      return -1;
    }

    enclave_dev = argv[1];
    enclave_fd = open(enclave_dev, O_RDONLY);
    if (enclave_fd == -1) {
      printf("Error opening enclave device: %s\n", enclave_dev);
      return -1;
    }

    cons_fd = ioctl(enclave_fd, PISCES_ENCLAVE_CONS_CONNECT, NULL); 

    close(enclave_fd);

    if (cons_fd < 0) {
      printf("Error opening enclave console\n");
      return -1;
    }

    while (1) {
        int ret;
        int bytes_read = 0;

        bytes_read = read(cons_fd, inbuf, INBUF_SIZE);
        if (bytes_read == -1) {
          printf("Console error\n");
          return -1;
        }

        if (bytes_read == 0) {
          break;
        }

        inbuf[bytes_read] = '\0';
        printf("%s", inbuf);
    }

    close(cons_fd);

    return 0;
}


