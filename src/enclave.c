/* Pisces Enclave 
 * (c) 2013, Jack Lange, (jacklange@cs.pitt.edu)
 * (c) 2013, Jiannan Ouyang, (ouyang@cs.pitt.edu)
 */


#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/errno.h>
#include <linux/anon_inodes.h>

#include "pisces.h"
#include "enclave.h"
#include "mm.h"
#include "pisces_cons.h"
#include "boot.h"


#include "pgtables.h"


static struct pisces_enclave * enclave_map[MAX_ENCLAVES] = {[0 ... MAX_ENCLAVES - 1] = 0};
extern struct class * pisces_class;
extern int pisces_major_num;

static int enclave_open(struct inode * inode, struct file * filp) {
    struct pisces_enclave * enclave = container_of(inode->i_cdev, struct pisces_enclave, cdev);
    filp->private_data = enclave;
    return 0;
}

static ssize_t enclave_read(struct file *file, char __user *buffer,
			   size_t length, loff_t *offset) {

    return console_read(file, buffer, length, offset);
}


static ssize_t enclave_write(struct file *file, const char __user *buffer,
			    size_t length, loff_t *offset) {
    printk(KERN_INFO "Write\n");
    return -EINVAL;
}



static long enclave_ioctl(struct file * filp,
			unsigned int ioctl, unsigned long arg) {


    return 0;
}


static struct file_operations enclave_fops = {
    .owner = THIS_MODULE,
    .unlocked_ioctl = enclave_ioctl,
    .compat_ioctl = enclave_ioctl,
    .open = enclave_open,
    .read = enclave_read, 
    .write = enclave_write,
};

struct pisces_enclave *  pisces_create_enclave(struct pisces_image * img) {

    struct pisces_enclave * enclave = NULL;

    printk(KERN_DEBUG "Creating Pisces Enclave\n");

    enclave = kmalloc(sizeof(struct pisces_enclave), GFP_KERNEL);
    
    if (IS_ERR(enclave)) {
	printk(KERN_ERR "Could not allocate enclave state\n");
	return NULL;
    }

    memset(enclave, 0, sizeof(struct pisces_enclave));

    enclave->tmp_image_ptr = img;

    enclave->kern_path = img->kern_path;
    enclave->initrd_path = img->initrd_path;
    enclave->kern_cmdline = img->cmd_line;

    enclave->bootmem_size = 128 * 1024 * 1024;
    enclave->bootmem_addr_pa = pisces_alloc_pages((128 * 1024 * 1024) / PAGE_SIZE_4KB);
    memset(__va(enclave->bootmem_addr_pa), 0, 128 * 1024 * 1024);


    enclave->dev = MKDEV(pisces_major_num, 1);

    cdev_init(&(enclave->cdev), &enclave_fops);

    enclave->cdev.owner = THIS_MODULE;
    enclave->cdev.ops = &enclave_fops;


    if (cdev_add(&(enclave->cdev), enclave->dev, 1)) {
	printk(KERN_ERR "Fails to add cdev\n");
	kfree(enclave);
	return NULL;
    }

    if (device_create(pisces_class, NULL, enclave->dev, enclave, "pisces-enclave%d", MINOR(enclave->dev)) == NULL){
	printk(KERN_ERR "Fails to create device\n");
	cdev_del(&(enclave->cdev));
	kfree(enclave);
	return NULL;
    }

    printk("Enclave created at /dev/pisces-enclave%d\n", MINOR(enclave->dev));

    return enclave;
}


int pisces_launch_enclave(struct pisces_enclave * enclave) {

    enclave->boot_cpu = 1;

    if (setup_boot_params(enclave) == -1) {
	printk(KERN_ERR "Error setting up boot environment\n");

	return -1;
    }


    if (boot_enclave(enclave) == -1) {
	printk(KERN_ERR "Error booting enclave\n");
	return -1;
    }

    return 0;

}



void pisces_free_enclave(struct pisces_enclave * enclave) {

    pisces_free_pages(enclave->bootmem_addr_pa, (128 * 1024 * 1024) / PAGE_SIZE_4KB);
    kfree(enclave);

    return;
}
