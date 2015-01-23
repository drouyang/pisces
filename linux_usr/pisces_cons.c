/*
 * Pisces Enclave Console Utility
 * (c) Jiannan Ouyang, 2013
 */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "pisces.h"
#define INBUF_SIZE 4096

int main(int argc, char* argv[]) {
    int  cons_fd;
    char inbuf[INBUF_SIZE + 1];

    if (argc < 2) {
      printf("usage: pisces_cons <enclave_device>\n");
      return -1;
    }

    cons_fd = pisces_get_cons_fd(get_pisces_id_from_path(argv[1]));

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


