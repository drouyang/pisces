/* Pisces Memory Management
 * (c) 2013, Jiannan Ouyang, (ouyang@cs.pitt.edu)
 * (c) 2013, Jack Lange, (jacklange@cs.pitt.edu)
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>    /* device file */
#include <linux/types.h>    /* dev_t */
#include <linux/kdev_t.h>    /* MAJOR MINOR MKDEV */
#include <linux/device.h>    /* udev */
#include <linux/cdev.h>    /* cdev_init cdev_add */
#include <linux/moduleparam.h>    /* module_param */
#include <linux/stat.h>    /* perms */
#include <asm/uaccess.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#include "pisces.h"      /* device file ioctls*/
#include "pisces_mod.h"      
#include "domain_xcall.h"      
#include "mm.h"
#include "enclave.h"
#include "boot.h"
#include "boot_params.h"

struct cdev c_dev;  
static dev_t dev_num; // <major , minor> 
static struct class *cl; // <major , minor> 

struct proc_dir_entry * pisces_proc_dir = NULL;


// TEMPORARY PLACE HOLDER FOR ENCLAVE
// THIS SHOULD GO IN THE PRIVATE DATA STRUCTURE OF THE FILP ASSOCIATED WITH A DYNAMICALLY CREATED DEVICE FILE
struct pisces_enclave * enclave = NULL;


static int device_open(struct inode *inode, struct file *file)
{
    //printk(KERN_INFO "Open\n");

    //try_module_get(THIS_MODULE);
    return 0;
}
static int device_release(struct inode *inode, struct file *file)
{
    //printk(KERN_INFO "Release\n");

    //module_put(THIS_MODULE);
    return 0;
}


// For now just forward reads to the console, and we'll just access the global enclave
#include "pisces_cons.h"
static ssize_t device_read(struct file *file, char __user *buffer,
			   size_t length, loff_t *offset) {

    return console_read(file, buffer, length, offset);
}


static ssize_t device_write(struct file *file, const char __user *buffer,
			    size_t length, loff_t *offset) {
    printk(KERN_INFO "Write\n");
    return -EINVAL;
}


static long device_ioctl(struct file * file, unsigned int ioctl,
			 unsigned long arg) {
    void __user * argp = (void __user *)arg;


    switch (ioctl) {
        case P_IOCTL_ADD_MEM: {
            struct memory_range reg;
            uintptr_t base_addr = 0;        
            u64 num_pages = 0;

            if (copy_from_user(&reg, argp, sizeof(struct memory_range))) {
		printk(KERN_ERR "Copying memory region from user space\n");
                return -EFAULT;
            }

            base_addr = (uintptr_t)reg.base_addr;
            num_pages = reg.pages;

	    if (pisces_add_mem(base_addr, num_pages) != 0) {
		printk(KERN_ERR "Error adding memory to pisces (base_addr=%p, pages=%llu)\n", 
		       (void *)base_addr, num_pages);
		return -EFAULT;
	    }


            break;
        }

	case P_IOCTL_LOAD_IMAGE: {
	    struct pisces_image * img = kmalloc(sizeof(struct pisces_image), GFP_KERNEL);

	    if (IS_ERR(img)) {
		printk(KERN_ERR "Could not allocate space for pisces image\n");
		return -EFAULT;
	    }

	    if (copy_from_user(img, argp, sizeof(struct pisces_image))) {
		printk(KERN_ERR "Error copying pisces from user space\n");
		return -EFAULT;
	    }

	    printk("Creating Pisces Image\n");

	    enclave = pisces_create_enclave(img);

	    if (enclave == NULL) {
		printk(KERN_ERR "Error creating Pisces Enclave\n");
		return -EFAULT;
	    }

	    break;
	}
	case P_IOCTL_LAUNCH_ENCLAVE: {

            printk(KERN_DEBUG "Launch Pisces Enclave\n");
	    pisces_launch_enclave(enclave);

            break;
	}

        case P_IOCTL_PRINT_IMAGE:
            {
                long  *p = (long *)__va(enclave->bootmem_addr_pa);
                //long *p = (long *)0x8000000;
                int t = 10;
                printk(KERN_INFO "PISCES: physical  address 0x%lx\n", enclave->bootmem_addr_pa);
                while (t > 0) {
                    printk(KERN_INFO "%p\t", (void *)*p);
                    p++;
                    t--;
                }

                break;
            
            }
        case P_IOCTL_EXIT:
            domain_xcall_exit();
            break;


	default:
	    printk(KERN_ERR "Invalid Pisces IOCTL: %d\n", ioctl);
	    return -EINVAL;

    }
    return 0;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .read = device_read,
    .write = device_write,
    .unlocked_ioctl = device_ioctl,
    .compat_ioctl = device_ioctl,
    .open = device_open,
    .release = device_release
};





static int 
dbg_mem_show(struct seq_file * s, void * v) {

    struct pisces_boot_params * boot_params = (struct pisces_boot_params *)__va(enclave->bootmem_addr_pa);
  
    seq_printf(s, "%s: %p", boot_params->init_dbg_buf, (void *)*(((u64*)(boot_params->init_dbg_buf)) + 1));
    
  
    return 0;
}


static int dbg_proc_open(struct inode * inode, struct file * filp) {
    struct proc_dir_entry * proc_entry = PDE(inode);
    return single_open(filp, dbg_mem_show, proc_entry->data);
}


static struct file_operations dbg_proc_ops = {
    .owner = THIS_MODULE,
    .open = dbg_proc_open, 
    .read = seq_read,
    .llseek = seq_lseek, 
    .release = single_release,
};




// return device major number, -1 if failed
int pisces_init(void) {
    
    pisces_proc_dir = proc_mkdir(PISCES_PROC_DIR, NULL);


    pisces_trampoline_init();

    if (pisces_mem_init() == -1) {
	printk(KERN_ERR "Error initializing Pisces Memory Management\n");
	return -1;
    }

    if (alloc_chrdev_region(&dev_num, 0, 1, "pisces") < 0) {
	printk(KERN_ERR "Error allocating Pisces Char device region\n");
	return -1;
    }

    //printk(KERN_INFO "<Major, Minor>: <%d, %d>\n", MAJOR(dev_num), MINOR(dev_num));

    if ((cl = class_create(THIS_MODULE, "pisces")) == NULL) {
	printk(KERN_ERR "Error creating Pisces Device Class\n");

        unregister_chrdev_region(dev_num, 1);
        return -1;
    }
    
    if (device_create(cl, NULL, dev_num, NULL, "pisces") == NULL) {
	printk(KERN_ERR "Error creating Pisces Device\n");

        class_destroy(cl);
        unregister_chrdev_region(dev_num, 1);
        return -1;
    }

    cdev_init(&c_dev, &fops);

    if (cdev_add(&c_dev, dev_num, 1) == -1) {
	printk(KERN_ERR "Error Adding Pisces CDEV\n");

        device_destroy(cl, dev_num);
        class_destroy(cl);
        unregister_chrdev_region(dev_num, 1);
        return -1;
    }


    {
	struct proc_dir_entry * dbg_entry = NULL;


	dbg_entry = create_proc_entry("pisces-dbg", 0444, NULL);
	if (dbg_entry) {
	    dbg_entry->proc_fops = &dbg_proc_ops;
	    dbg_entry->data = &enclave;
	} else {
	    printk(KERN_ERR "Error creating memoryproc file\n");
	}

    }



    if (domain_xcall_init() < 0) {
        printk(KERN_INFO "PISCES: domain_xcall_init failed\n");
        return -1;
    }

    return 0;
}

void pisces_exit(void) {
    cdev_del(&c_dev);
    device_destroy(cl, dev_num);
    class_destroy(cl);
    unregister_chrdev_region(dev_num, 1);

    remove_proc_entry(PISCES_PROC_DIR, NULL);
    
}




module_init(pisces_init);
module_exit(pisces_exit);

MODULE_LICENSE("GPL");
