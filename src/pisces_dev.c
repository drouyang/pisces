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

#include "pisces_dev.h"      /* device file ioctls*/
#include "pisces_mod.h"      
#include "pisces_loader.h"      
#include "pisces.h"      
#include "domain_xcall.h"      
#include "buddy.h"
#include "numa.h"
#include "enclave.h"

struct cdev c_dev;  
static dev_t dev_num; // <major , minor> 
static struct class *cl; // <major , minor> 

static struct buddy_memzone ** memzones = NULL;
static struct proc_dir_entry * pisces_proc_dir = NULL;


//static cpumask_t avail_cpus;

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

static ssize_t device_read(struct file *file, char __user *buffer,
			   size_t length, loff_t *offset) {
    return -EINVAL;
}


static ssize_t device_write(struct file *file, const char __user *buffer,
			    size_t length, loff_t *offset) {
    printk(KERN_INFO "Write\n");
    return -EINVAL;
}


static long device_ioctl(struct file * file, unsigned int ioctl,
			 unsigned long arg) {
    void __user * argp = (void __user *)arg;
    long ret;

    switch (ioctl) {
        case P_IOCTL_ADD_MEM: {
            struct memory_range reg;
            uintptr_t base_addr = 0;        
            u32 num_pages = 0;
            int node_id = 0;
            int pool_order = 0;

            if (copy_from_user(&reg, argp, sizeof(struct memory_range))) {
		printk(KERN_ERR "Copying memory region from user space\n");
                return -EFAULT;
            }

            base_addr = (uintptr_t)reg.base_addr;
            num_pages = reg.pages;

            node_id = numa_addr_to_node(base_addr);
            
            if (node_id == -1) {
                printk(KERN_ERR "Error locating node for addr %p\n", (void *)base_addr);
                return -1;
            }
              
	    printk(KERN_DEBUG "Managing %dMB of memory starting at %llu (%lluMB)\n", 
                   (unsigned int)(num_pages * PAGE_SIZE) / (1024 * 1024), 
                   (unsigned long long)base_addr, 
                   (unsigned long long)(base_addr / (1024 * 1024)));
          
          
            //   pool_order = fls(num_pages); 
            pool_order = get_order(num_pages * PAGE_SIZE) + PAGE_SHIFT;
            buddy_add_pool(memzones[node_id], base_addr, pool_order);

            printk(KERN_DEBUG "%p on node %d\n", (void *)base_addr, numa_addr_to_node(base_addr));

            break;
        }


        case P_IOCTL_PING:
            printk(KERN_INFO "PISCES: mem_base 0x%lx, mem_len 0x%lx, cpuid %lu, kernel_path '%s'\n", 
                    mem_base, mem_len, cpu_id, kernel_path);
            break;

        case P_IOCTL_PREPARE_SECONDARY:

            printk(KERN_INFO "PISCES: setup bootstrap page table for [0x%lx, 0x%lx)\n", 
                    mem_base, mem_base+mem_len);
            pgtable_setup_ident(mem_base, mem_len);

        case P_IOCTL_LOAD_IMAGE: {
	    struct pisces_image * img = kmalloc(sizeof(struct pisces_image), GFP_KERNEL);;

	    if (IS_ERR(img)) {
		printk(KERN_ERR "Could not allocate space for pisces image\n");
		return -EFAULT;
	    }

	    if (copy_from_user(img, argp, sizeof(struct pisces_image))) {
		printk(KERN_ERR "Error copying pisces from user space\n");
		return -EFAULT;
	    }

	    printk("Creating Pisces Image\n");

	    ret = pisces_create_enclave(img);

	    if (ret == -1) {
		printk(KERN_ERR "Error creating Pisces Enclave\n");
		return -EIO;
	    }

            break;
	}
        case P_IOCTL_START_SECONDARY:
            printk(KERN_INFO "PISCES: start secondary cpu %ld\n", cpu_id);
            kick_offline_cpu();

            break;

        case P_IOCTL_PRINT_IMAGE:
            {
                long *p = (long *)__va(mem_base);
                //long *p = (long *)0x8000000;
                int t=10;
                printk(KERN_INFO "PISCES: physicall address 0x%lx\n", mem_base);
                while (t>0) {
                    printk(KERN_INFO "%p\t", (void *)*p);
                    p++;
                    t--;
                }

                break;
            
            }
        case P_IOCTL_EXIT:
            domain_xcall_exit();
            break;


	    //	case P_IOCTL_CONS_CONNECT:
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

// return device major number, -1 if failed
int device_init(void) {
    
    pisces_proc_dir = proc_mkdir(PISCES_PROC_DIR, NULL);


    {
        int num_nodes = numa_num_nodes();
        int node_id = 0;
        
        memzones = kmalloc(GFP_KERNEL, sizeof(struct buddy_memzone *) * num_nodes);
        memset(memzones, 0, sizeof(struct buddy_memzone *) * num_nodes);

        for (node_id = 0; node_id < num_nodes; node_id++) {
            struct buddy_memzone * zone = NULL;
            
            printk("Initializing Zone %d\n", node_id);
            zone = buddy_init(get_order(0x40000000) + PAGE_SHIFT, PAGE_SHIFT, node_id, pisces_proc_dir);

            if (zone == NULL) {
                printk(KERN_ERR "Could not initialization memory management for node %d\n", node_id);
                return -1;
            }

            printk("Zone initialized, Adding seed region (order=%d)\n", 
               (get_order(0x40000000) + PAGE_SHIFT));

            //      buddy_add_pool(zone, seed_addrs[node_id], (MAX_ORDER - 1) + PAGE_SHIFT);

            memzones[node_id] = zone;
        }
    }


    if (alloc_chrdev_region(&dev_num, 0, 1, "pisces") < 0) {
	return -1;
    }
    //printk(KERN_INFO "<Major, Minor>: <%d, %d>\n", MAJOR(dev_num), MINOR(dev_num));

    if ((cl = class_create(THIS_MODULE, "pisces")) == NULL) {
        unregister_chrdev_region(dev_num, 1);
        return -1;
    }
    
    if (device_create(cl, NULL, dev_num, NULL, "pisces") == NULL) {
        class_destroy(cl);
        unregister_chrdev_region(dev_num, 1);
        return -1;
    }

    cdev_init(&c_dev, &fops);

    if (cdev_add(&c_dev, dev_num, 1) == -1) {
        device_destroy(cl, dev_num);
        class_destroy(cl);
        unregister_chrdev_region(dev_num, 1);
        return -1;
    }


    return 0;
}

void device_exit(void) {
    cdev_del(&c_dev);
    device_destroy(cl, dev_num);
    class_destroy(cl);
    unregister_chrdev_region(dev_num, 1);
}
