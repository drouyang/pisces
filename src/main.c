/* 
 * Pisces main interface
 * (c) 2013, Jiannan Ouyang, (ouyang@cs.pitt.edu)
 * (c) 2013, Jack Lange, (jacklange@cs.pitt.edu)
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>             /* device file */
#include <linux/types.h>          /* dev_t */
#include <linux/kdev_t.h>         /* MAJOR MINOR MKDEV */
#include <linux/device.h>         /* udev */
#include <linux/cdev.h>           /* cdev_init cdev_add */
#include <linux/moduleparam.h>    /* module_param */
#include <linux/stat.h>           /* perms */
#include <asm/uaccess.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/version.h>

#include "pisces.h"                /* device file ioctls*/
#include "linux_syms.h"
#include "pisces_mod.h"
#include "enclave.h"
#include "ipi.h"
#include "boot.h"
#include "pisces_boot_params.h"

int                      pisces_major_num = 0;
struct class           * pisces_class     = NULL;
static struct cdev       pisces_cdev;  
struct proc_dir_entry  * pisces_proc_dir  = NULL;

extern struct pisces_enclave * enclave_map[MAX_ENCLAVES];



static int 
device_open(struct inode * inode, 
	    struct file  * file)
{
    //printk(KERN_INFO "Open\n");

    //try_module_get(THIS_MODULE);
    return 0;
}


static int 
device_release(struct inode * inode,
	       struct file  * file)
{
    //printk(KERN_INFO "Release\n");

    //module_put(THIS_MODULE);
    return 0;
}

static ssize_t 
device_read(struct file * file, 
	    char __user * buffer,
	    size_t        length, 
	    loff_t      * offset)
{
    printk(KERN_INFO "Read\n");
    return -EINVAL;
}


static ssize_t 
device_write(struct file       * file,
	     const char __user * buffer,
	     size_t              length, 
	     loff_t            * offset) 
{
    printk(KERN_INFO "Write\n");
    return -EINVAL;
}





static long 
device_ioctl(struct file  * file,
	     unsigned int   ioctl,
	     unsigned long  arg) 
{
    void __user * argp = (void __user *)arg;


    switch (ioctl) {

        case PISCES_LOAD_IMAGE: {
            struct pisces_image * img = kmalloc(sizeof(struct pisces_image), GFP_KERNEL);
            int enclave_idx           = -1;

            if (IS_ERR(img)) {
                printk(KERN_ERR "Could not allocate space for pisces image\n");
                return -EFAULT;
            }

            if (copy_from_user(img, argp, sizeof(struct pisces_image))) {
                printk(KERN_ERR "Error copying pisces image from user space\n");
                return -EFAULT;
            }

            printk("Creating Enclave\n");

            enclave_idx = pisces_enclave_create(img);

            if (enclave_idx == -1) {
                printk(KERN_ERR "Error creating Pisces Enclave\n");
                return -EFAULT;
            }

            return enclave_idx;
            break;
        }

	case PISCES_FREE_ENCLAVE: {
	    int enclave_idx = arg;

	    pisces_enclave_free(enclave_map[enclave_idx]);

	    break;
	}
        default:
            printk(KERN_ERR "Invalid Pisces IOCTL: %d\n", ioctl);
            return -EINVAL;
    }

    return 0;
}



static struct file_operations fops = {
    .owner            = THIS_MODULE,
    .read             = device_read,
    .write            = device_write,
    .unlocked_ioctl   = device_ioctl,
    .compat_ioctl     = device_ioctl,
    .open             = device_open,
    .release          = device_release
};




static int 
dbg_mem_show(struct seq_file * s,
	     void            * v) 
{
    struct pisces_boot_params * boot_params = NULL;

    boot_params = (struct pisces_boot_params *)__va(enclave_map[0]->bootmem_addr_pa);
    seq_printf(s, "%s: %p\n", boot_params->init_dbg_buf, (void *)*(((u64*)(boot_params->init_dbg_buf)) + 1));
    seq_printf(s, "tramp_status: %p\n", (void *)*(((u64 *)(boot_params))));

    return 0;
}


static int 
dbg_proc_open(struct inode * inode, 
	      struct file  * filp) 
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0)
    void * data = PDE(inode)->data;
#else 
    void * data = PDE_DATA(inode);
#endif

    return single_open(filp, dbg_mem_show, data);
}


static struct file_operations dbg_proc_ops = {
    .owner     = THIS_MODULE,
    .open      = dbg_proc_open, 
    .read      = seq_read,
    .llseek    = seq_lseek, 
    .release   = single_release,
};




// return device major number, -1 if failed
int 
pisces_init(void) 
{
    dev_t dev_num   = MKDEV(0, 0); // <major , minor> 

    pisces_proc_dir = proc_mkdir(PISCES_PROC_DIR, NULL);

    if (pisces_linux_symbol_init() != 0) {
        printk(KERN_ERR "Could not initialize Pisces Linux symbol\n");
        return -1;
    }

    if (pisces_init_trampoline() != 0) {
	printk(KERN_ERR "Could not initialize trampoline\n");
	return -1;
    }

    if (pisces_ipi_init() != 0) {
        printk(KERN_ERR "Could not initialize Pisces IPI handler\n");
        return -1;
    }


    if (alloc_chrdev_region(&dev_num, 0, MAX_ENCLAVES + 1, "pisces") < 0) {
        printk(KERN_ERR "Error allocating Pisces Char device region\n");
        return -1;
    }

    pisces_major_num = MAJOR(dev_num);
    dev_num          = MKDEV(pisces_major_num, MAX_ENCLAVES + 1);

    //printk(KERN_INFO "<Major, Minor>: <%d, %d>\n", MAJOR(dev_num), MINOR(dev_num));

    if ((pisces_class = class_create(THIS_MODULE, "pisces")) == NULL) {
        printk(KERN_ERR "Error creating Pisces Device Class\n");

        unregister_chrdev_region(dev_num, 1);
        return -1;
    }

    if (device_create(pisces_class, NULL, dev_num, NULL, "pisces") == NULL) {
        printk(KERN_ERR "Error creating Pisces Device\n");

        class_destroy(pisces_class);
        unregister_chrdev_region(dev_num, MAX_ENCLAVES + 1);
        return -1;
    }

    cdev_init(&pisces_cdev, &fops);

    if (cdev_add(&pisces_cdev, dev_num, 1) == -1) {
        printk(KERN_ERR "Error Adding Pisces CDEV\n");

        device_destroy(pisces_class, dev_num);
        class_destroy(pisces_class);
        unregister_chrdev_region(dev_num, MAX_ENCLAVES + 1);
        return -1;
    }


    {
        struct proc_dir_entry * dbg_entry = NULL;


#if LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0)
        dbg_entry = create_proc_entry("pisces-dbg", 0444, pisces_proc_dir);

        if (dbg_entry) {
            dbg_entry->proc_fops = &dbg_proc_ops;
            dbg_entry->data      = enclave_map[0];
        } else {
            printk(KERN_ERR "Error creating memoryproc file\n");
        }
#else
	dbg_entry = proc_create_data("pisces-dbg", 0444, pisces_proc_dir, &dbg_proc_ops, enclave_map[0]);

	if (!dbg_entry) {
	    printk(KERN_ERR "Error creating memory proc file\n");
	}
#endif


    }




    return 0;
}

void 
pisces_exit(void) 
{
    dev_t dev_num = MKDEV(pisces_major_num, MAX_ENCLAVES + 1);

    unregister_chrdev_region(MKDEV(pisces_major_num, 0), MAX_ENCLAVES + 1);

    cdev_del(&pisces_cdev);
    device_destroy(pisces_class, dev_num);
    class_destroy(pisces_class);

    remove_proc_entry(PISCES_PROC_DIR, NULL);
}



module_init(pisces_init);
module_exit(pisces_exit);

MODULE_LICENSE("GPL");
