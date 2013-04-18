#ifndef _PISCES_DEV_H_
#define _PISCES_DEV_H_

/* 
 * The name of the device file 
 */
#define DEVICE_NAME "pisces"

/* Pisces global ioctls */

#define G_IOCTL_MIN    1000
#define G_IOCTL_PING    1000

// Initialized mem_base and mem_size params before prepare secondary
// This will setup physical memory and identity mapping for the secondary cpu
#define G_IOCTL_PREPARE_SECONDARY    1001

#define G_IOCTL_LOAD_IMAGE    1002

// Boot secondary cpu via INIT, secondary cpu will jump to hooker() in long mode
#define G_IOCTL_START_SECONDARY    1003

#define G_IOCTL_PRINT_IMAGE    1004

#define G_IOCTL_READ_CONSOLE_BUFFER 1005

#define G_IOCTL_MAX    1005

int device_init(void);
void device_exit(void);
#endif
