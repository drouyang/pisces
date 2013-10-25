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
#include <linux/uaccess.h>
#include <linux/fs.h>    /* device file */
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#include "pisces.h"
#include "enclave.h"
#include "pisces_ctrl.h"
#include "boot.h"


#include "pgtables.h"


extern struct proc_dir_entry * pisces_proc_dir;
extern struct class * pisces_class;
extern int pisces_major_num;


struct pisces_enclave * enclave_map[MAX_ENCLAVES] = {[0 ... MAX_ENCLAVES - 1] = 0};

static int alloc_enclave_index(struct pisces_enclave * enclave) {
    int i = 0;
    for (i = 0; i < MAX_ENCLAVES; i++) {
        if (enclave_map[i] == NULL) {
            enclave_map[i] = enclave;
            return i;
        }
    }

    return -1;
}

static void free_enclave_index(int idx) {
    enclave_map[idx] = NULL;
}



static int pisces_enclave_launch(struct pisces_enclave * enclave);
static void pisces_enclave_free(struct pisces_enclave * enclave);




static int enclave_open(struct inode * inode, struct file * filp) {
    struct pisces_enclave * enclave = container_of(inode->i_cdev, struct pisces_enclave, cdev);
    filp->private_data = enclave;
    return 0;
}


static ssize_t enclave_read(struct file *filp, char __user *buffer,
        size_t length, loff_t *offset) {


    return 0;
}


static ssize_t enclave_write(struct file *filp, const char __user *buffer,
        size_t length, loff_t *offset) {


    return 0;
}



static long enclave_ioctl(struct file * filp,
        unsigned int ioctl, unsigned long arg) {
    void __user * argp = (void __user *)arg;
    struct pisces_enclave * enclave = (struct pisces_enclave *)filp->private_data;
    int ret = 0;

    switch (ioctl) {


        case PISCES_ENCLAVE_LAUNCH:
            {
		struct enclave_boot_env boot_env;
		
		memset(&boot_env, 0, sizeof(struct enclave_boot_env));

		if (copy_from_user(&boot_env, argp, sizeof(struct enclave_boot_env))) {
		    printk(KERN_ERR "Error copying pisces image from user space\n");
		    return -EFAULT;
		}

		/* We need to check that these values are legit */
		enclave->bootmem_addr_pa = boot_env.base_addr;
		enclave->bootmem_size = (boot_env.pages * PAGE_SIZE);
		enclave->boot_cpu = boot_env.cpu_id;

		pisces_enclave_add_cpu(enclave, boot_env.cpu_id);
		pisces_enclave_add_mem(enclave, boot_env.base_addr, boot_env.pages);

                printk(KERN_DEBUG "Launch Pisces Enclave (cpu=%d) (bootmem=%p)\n", 
		       enclave->boot_cpu, (void *)enclave->bootmem_addr_pa);

                ret = pisces_enclave_launch(enclave);

                break;
            }

        case PISCES_ENCLAVE_CONS_CONNECT:
            {
                printk(KERN_DEBUG "Open enclave console...\n");
                ret = pisces_cons_connect(enclave);
                break;
            }

        case PISCES_ENCLAVE_CTRL_CONNECT:
            {
                printk("Connecting Ctrl Channel\n");
                ret = pisces_ctrl_connect(enclave);
                break;

            }


    }

    return ret;
}


static int 
proc_mem_show(struct seq_file * s, void * v) {
    struct pisces_enclave * enclave = s->private;
    struct enclave_mem_block * iter = NULL;
    int i = 0;

    if (IS_ERR(enclave)) {
	seq_printf(s, "NULL ENCLAVE\n");
	return 0;
    }

    seq_printf(s, "Num Memory Blocks: %d\n", enclave->memdesc_num);

    list_for_each_entry(iter, &(enclave->memdesc_list), node) {
	seq_printf(s, "%d: %p - %p\n", i, 
		   (void *)iter->base_addr,
		   (void *)(iter->base_addr + (iter->pages * 4096)));
	i++;
    }

    return 0;
}

static int 
proc_mem_open(struct inode * inode, struct file * filp) {
    struct proc_dir_entry * proc_entry = PDE(inode);
    return single_open(filp, proc_mem_show, proc_entry->data);
}

static int 
proc_cpu_show(struct seq_file * s, void * v) {
    struct pisces_enclave * enclave = s->private;
    int cpu_iter;

    if (IS_ERR(enclave)) {
	seq_printf(s, "NULL ENCLAVE\n");
	return 0;
    }

    seq_printf(s, "Num CPUs: %d\n", enclave->num_cpus);

    for_each_cpu(cpu_iter, &(enclave->assigned_cpus)) {
	seq_printf(s, "CPU %d\n", cpu_iter);
    }
 
    return 0;
}

static int 
proc_cpu_open(struct inode * inode, struct file * filp) {
    struct proc_dir_entry * proc_entry = PDE(inode);
    return single_open(filp, proc_cpu_show, proc_entry->data);
}



static struct file_operations enclave_fops = {
    .owner = THIS_MODULE,
    .unlocked_ioctl = enclave_ioctl,
    .compat_ioctl = enclave_ioctl,
    .open = enclave_open,
    .read = enclave_read, 
    .write = enclave_write,
};


static struct file_operations proc_mem_fops = {
    .owner = THIS_MODULE, 
    .open = proc_mem_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
};


static struct file_operations proc_cpu_fops = {
    .owner = THIS_MODULE, 
    .open = proc_cpu_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
};


int pisces_enclave_create(struct pisces_image * img) {

    struct pisces_enclave * enclave = NULL;
    int enclave_idx = -1;

    printk(KERN_DEBUG "Creating Pisces Enclave\n");

    enclave = kmalloc(sizeof(struct pisces_enclave), GFP_KERNEL);

    if (IS_ERR(enclave)) {
        printk(KERN_ERR "Could not allocate enclave state\n");
        return -1;
    }

    memset(enclave, 0, sizeof(struct pisces_enclave));

    enclave_idx = alloc_enclave_index(enclave);

    if (enclave_idx == -1) {
        printk(KERN_ERR "Too many enclaves already created. Cannot create a new one\n");
        kfree(enclave);
        return -1;
    }

    enclave->state = ENCLAVE_LOADED;

    enclave->kern_path = img->kern_path;
    enclave->initrd_path = img->initrd_path;
    enclave->kern_cmdline = img->cmd_line;

    enclave->id = enclave_idx;
    
    INIT_LIST_HEAD(&(enclave->memdesc_list));

    enclave->dev = MKDEV(pisces_major_num, enclave_idx);
    enclave->memdesc_num = 0;

    cpumask_clear(&(enclave->assigned_cpus));
    enclave->num_cpus = 0;

    cdev_init(&(enclave->cdev), &enclave_fops);

    enclave->cdev.owner = THIS_MODULE;
    enclave->cdev.ops = &enclave_fops;

    

    if (cdev_add(&(enclave->cdev), enclave->dev, 1)) {
        printk(KERN_ERR "Fails to add cdev\n");
        kfree(enclave);
        return -1;
    }

    if (device_create(pisces_class, NULL, enclave->dev, enclave, "pisces-enclave%d", MINOR(enclave->dev)) == NULL){
        printk(KERN_ERR "Fails to create device\n");
        cdev_del(&(enclave->cdev));
        kfree(enclave);
        return -1;
    }

    /* Setup proc entries */
    {
	char name[128];
	struct proc_dir_entry * mem_entry = NULL;
	struct proc_dir_entry * cpu_entry = NULL;

	memset(name, 0, 128);
	snprintf(name, 128, "enclave-%d", enclave->id);
	
	enclave->proc_dir = proc_mkdir(name, pisces_proc_dir);

	mem_entry = create_proc_entry("memory", 0444, enclave->proc_dir);

	if (mem_entry) {
	    mem_entry->proc_fops = &proc_mem_fops;
	    mem_entry->data = enclave;
	}
	
	cpu_entry = create_proc_entry("cpus", 0444, enclave->proc_dir);
	
	if (cpu_entry) {
	    cpu_entry->proc_fops = &proc_cpu_fops;
	    cpu_entry->data = enclave;
	}
    }

    printk("Enclave created at /dev/pisces-enclave%d\n", MINOR(enclave->dev));

    return enclave_idx;
}


static int pisces_enclave_launch(struct pisces_enclave * enclave) {

    if (setup_boot_params(enclave) == -1) {
        printk(KERN_ERR "Error setting up boot environment\n");
        return -1;
    }


    if (boot_enclave(enclave) == -1) {
        printk(KERN_ERR "Error booting enclave\n");
        return -1;
    }
    enclave->state = ENCLAVE_RUNNING;

    return 0;

}



static void pisces_enclave_free(struct pisces_enclave * enclave) {

    free_enclave_index(enclave->id);
    kfree(enclave);

    return;
}



int pisces_enclave_add_cpu(struct pisces_enclave * enclave, u32 cpu_id) {

    if (cpumask_test_and_set_cpu(cpu_id, &(enclave->assigned_cpus))) {
	// CPU already present
	printk(KERN_ERR "Error tried to add an already present CPU (%d) to enclave %d\n", cpu_id, enclave->id);
	return -1;
    }

    enclave->num_cpus++;

    return 0;
}

int pisces_enclave_add_mem(struct pisces_enclave * enclave, u64 base_addr, u32 pages) {
    struct enclave_mem_block * memdesc = kmalloc(sizeof(struct enclave_mem_block), GFP_KERNEL);
    struct enclave_mem_block * iter = NULL;

    if (IS_ERR(memdesc)) {
	printk(KERN_ERR "Could not allocate memory descriptor\n");
	return -1;
    }

    memdesc->base_addr = base_addr;
    memdesc->pages = pages;

    if (enclave->memdesc_num == 0) {
	list_add(&(memdesc->node), &(enclave->memdesc_list));
    } else {

	list_for_each_entry(iter, &(enclave->memdesc_list), node) {
	    if (iter->base_addr > memdesc->base_addr) {
		list_add_tail(&(memdesc->node), &(iter->node));
		break;
	    } else if (list_is_last(&(iter->node), &(enclave->memdesc_list))) {
		list_add(&(memdesc->node), &(iter->node));
		break;
	    }
	}
    }

    enclave->memdesc_num++;
    return 0;
}
