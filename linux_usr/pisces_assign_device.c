/*  Assign PCI device to Pisces
 *  (c) Jiannan Ouyang, 2013
 */
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h> 
#include <sys/ioctl.h> 
#include <sys/stat.h> 
#include <sys/types.h> 
#include <unistd.h>
#include <string.h>

#include "../src/pisces.h"
#include <pet_ioctl.h>

static struct pisces_host_pci_bdf dev_info;

int main(int argc, char ** argv) {
    int v3_fd = 0;
    unsigned int bus = 0;
    unsigned int dev = 0;
    unsigned int func = 0;
    int ret = 0;

    if (argc < 3) {
	printf("Usage: ./pisces_assign_device <name> <bus> <dev> <func>\n");
	return -1;
    }

    bus = atoi(argv[2]);
    dev = atoi(argv[3]);
    func = atoi(argv[4]);

    strncpy(dev_info.name, argv[1], 128);
    dev_info.domain = 0;
    dev_info.bus = bus;
    dev_info.dev = dev;
    dev_info.func = func;
    
    ret = pet_ioctl_path("/dev/" DEVICE_NAME, PISCES_ASSIGN_DEVICE, &dev_info);
    if (ret != 0) {
    	printf("Device Assign Failed\n");
    }
}
