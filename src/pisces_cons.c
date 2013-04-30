/* Pisces Console / Userland interface
 * (c) 2013, Jiannan Ouyang (ouyang@cs.pitt.edu)
 * (c) 2013, Jack Lange (jacklange@cs.pitt.edu)
 */

#include "pisces_cons.h"



static ssize_t device_read(struct file *file, char __user *buffer,
			   size_t length, loff_t *offset) {
    int count = 0;

    *offset += count;
    return count;
}
