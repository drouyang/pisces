#include <linux/version.h>
#include <stdio.h>
#include <stdlib.h>

int main() {
    
    if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,33)) {
	printf("0\n");
    } else if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,39)) {
	printf("1\n");
    } else if (LINUX_VERSION_CODE < KERNEL_VERSION(3,5,0)) {
	printf("2\n");
    } else {
	printf("3\n");
    }

    return 0;
}
