#include <linux/version.h>
#include <stdio.h>
#include <stdlib.h>

int main() {
    
    if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,40)) {
	printf("1\n");
    } else {
	printf("0\n");
    }

    return 0;
}
