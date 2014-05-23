/* Host PCI User space tool
 *  (c) Jack Lange, 2012
 *  jacklange@cs.pitt.edu 
 */
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h> 
#include <sys/ioctl.h> 
#include <sys/stat.h> 
#include <sys/types.h> 
#include <unistd.h>
#include <string.h>


#include <pet_types.h>
#include <pet_ioctl.h>

#include "../src/pisces.h"
#include "../src/pisces_cmds.h"


int main(int argc, char ** argv) {
    int ctrl_fd;
    char * enclave_path = argv[1];
    struct pisces_pci_spec dev_info;
    unsigned int bus = 0;
    unsigned int dev = 0;
    unsigned int func = 0;
    int ret = 0;

    if (argc < 3) {
	printf("Usage: ./v3_pci <pisces_device> <name> <bus> <dev> <func>\n");
	return -1;
    }

    bus = atoi(argv[3]);
    dev = atoi(argv[4]);
    func = atoi(argv[5]);

    strncpy(dev_info.name, argv[2], 128);
    dev_info.bus = bus;
    dev_info.dev = dev;
    dev_info.func = func;
    
    printf("Registering %s at %d.%d:%d (%s)\n", dev_info.name, dev_info.bus, dev_info.dev, dev_info.func, 
	enclave_path);

    ctrl_fd = pet_ioctl_path(enclave_path, PISCES_ENCLAVE_CTRL_CONNECT, NULL);

    if (ctrl_fd < 0) {
        printf("Error opening enclave control channel (%s)\n", enclave_path);
        return -1;
    }

    ret = pet_ioctl_fd(ctrl_fd, ENCLAVE_CMD_ADD_V3_PCI, &dev_info);
    
    if (ret < 0) {
	printf("Error registering PCI device\n");
	return -1;
    }

}
