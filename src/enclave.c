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
#include <linux/version.h>
#include <linux/pci.h>

#include "pisces.h"
#include "enclave.h"
#include "enclave_ctrl.h"
#include "pisces_xpmem.h"
#include "boot.h"
#include "pisces_boot_params.h"


#include "pgtables.h"


extern struct proc_dir_entry * pisces_proc_dir;
extern struct class          * pisces_class;
extern int                     pisces_major_num;


struct pisces_enclave * enclave_map[MAX_ENCLAVES] = {[0 ... MAX_ENCLAVES - 1] = 0};

static int 
alloc_enclave_index(struct pisces_enclave * enclave) 
{
    int i = 0;

    for (i = 0; i < MAX_ENCLAVES; i++) {
        if (enclave_map[i] == NULL) {
            enclave_map[i] = enclave;
            return i;
        }
    }

    return -1;
}


static void 
free_enclave_index(int idx) 
{
    enclave_map[idx] = NULL;
}



static int pisces_enclave_launch(struct pisces_enclave * enclave);




static int 
enclave_open(struct inode * inode, 
	     struct file  * filp) 
{
    struct pisces_enclave * enclave = container_of(inode->i_cdev, struct pisces_enclave, cdev);
    filp->private_data = enclave;
    return 0;
}


static ssize_t 
enclave_read(struct file  * filp, 
	     char __user  * buffer,
	     size_t         length, 
	     loff_t       * offset) 
{
    return 0;
}


static ssize_t 
enclave_write(struct file        * filp,
	      const char __user  * buffer,
	      size_t               length, 
	      loff_t             * offset) 
{
    return 0;
}



static long 
enclave_ioctl(struct file  * filp,
	      unsigned int   ioctl,
	      unsigned long  arg) 
{
    void __user           * argp    = (void __user *)arg;
    struct pisces_enclave * enclave = (struct pisces_enclave *)filp->private_data;
    int ret = 0;

    mutex_lock(&(enclave->op_lock));
    {
	switch (ioctl) {

	    case PISCES_ENCLAVE_LAUNCH:
		{
		    struct enclave_boot_env boot_env;
		    
		    memset(&boot_env, 0, sizeof(struct enclave_boot_env));
		    
		    if (copy_from_user(&boot_env, argp, sizeof(struct enclave_boot_env))) {
			printk(KERN_ERR "Error copying pisces image from user space\n");
			ret = -EFAULT;
			break;
		    }
		    
		    /* We need to check that these values are legit */
		    enclave->bootmem_addr_pa =  boot_env.base_addr;
		    enclave->bootmem_size    =  boot_env.pages * PAGE_SIZE;
		    enclave->boot_cpu        =  boot_env.cpu_id;
		    
		    pisces_enclave_add_cpu(enclave, boot_env.cpu_id);
		    pisces_enclave_add_mem(enclave, boot_env.base_addr, boot_env.pages);
		    
		    
		    printk(KERN_DEBUG "Launch Pisces Enclave (cpu=%d) (bootmem=%p)\n", 
			   enclave->boot_cpu, 
			   (void *)enclave->bootmem_addr_pa);
		    
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
    }
    mutex_unlock(&(enclave->op_lock));

    return ret;
}


static int 
proc_mem_show(struct seq_file * file, 
	      void            * priv_data)
 {
    struct pisces_enclave    * enclave = file->private;
    struct enclave_mem_block * iter    = NULL;
    int i = 0;

    if (IS_ERR(enclave)) {
	seq_printf(file, "NULL ENCLAVE\n");
	return 0;
    }

    mutex_lock(&(enclave->op_lock));
    {
	seq_printf(file, "Num Memory Blocks: %d\n", enclave->memdesc_num);
	
	list_for_each_entry(iter, &(enclave->memdesc_list), node) 
	{
	    seq_printf(file, "%d: %p - %p\n", i, 
		       (void *)iter->base_addr,
		       (void *)(iter->base_addr + (iter->pages * 4096)));
	    i++;
	}

    }
    mutex_unlock(&(enclave->op_lock));

    return 0;
}


static int 
proc_mem_open(struct inode * inode, 
	      struct file  * filp) 
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0)
    void * data = PDE(inode)->data;
#else 
    void * data = PDE_DATA(inode);
#endif

    return single_open(filp, proc_mem_show, data);
}

static int 
proc_cpu_show(struct seq_file * file, 
	      void            * priv_data)
{
    struct pisces_enclave * enclave = file->private;
    int cpu_iter;

    if (IS_ERR(enclave)) {
	seq_printf(file, "NULL ENCLAVE\n");
	return 0;
    }

    mutex_lock(&(enclave->op_lock));
    {
	seq_printf(file, "Num CPUs: %d\n", enclave->num_cpus);
	
	for_each_cpu(cpu_iter, &(enclave->assigned_cpus)) {
	    seq_printf(file, "CPU %d\n", cpu_iter);
	}
    }
    mutex_unlock(&(enclave->op_lock));

    return 0;
}

static int 
proc_cpu_open(struct inode * inode, 
	      struct file  * filp) 
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0)
    void * data = PDE(inode)->data;
#else 
    void * data = PDE_DATA(inode);
#endif

    return single_open(filp, proc_cpu_show, data);
}



static int 
proc_pci_show(struct seq_file * file, 
	      void            * priv_data)
{
    struct pisces_enclave * enclave = file->private;
    struct pisces_pci_dev * dev     = NULL;

    if (IS_ERR(enclave)) {
	seq_printf(file, "NULL ENCLAVE\n");
	return 0;
    }

    mutex_lock(&(enclave->op_lock));
    {
	seq_printf(file, "PCI Devices: %d\n", enclave->pci_state.dev_num);
	
	list_for_each_entry(dev, &(enclave->pci_state.dev_list), dev_node) {
	    seq_printf(file, "%s: %.2x:%.2x.%x\n", 
		       dev->name,
		       dev->bus, 
		       PCI_SLOT(dev->devfn),
		       PCI_FUNC(dev->devfn));
	    
	}
    }
    mutex_unlock(&(enclave->op_lock));

    return 0;


}

static int 
proc_pci_open(struct inode * inode, 
	      struct file  * filp) 
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0)
    void * data = PDE(inode)->data;
#else 
    void * data = PDE_DATA(inode);
#endif

    return single_open(filp, proc_pci_show, data);
}



static struct file_operations enclave_fops = {
    .owner          = THIS_MODULE,
    .unlocked_ioctl = enclave_ioctl,
    .compat_ioctl   = enclave_ioctl,
    .open           = enclave_open,
    .read           = enclave_read, 
    .write          = enclave_write,
};


static struct file_operations proc_mem_fops = {
    .owner   = THIS_MODULE, 
    .open    = proc_mem_open,
    .read    = seq_read,
    .llseek  = seq_lseek,
    .release = single_release,
};


static struct file_operations proc_cpu_fops = {
    .owner   = THIS_MODULE, 
    .open    = proc_cpu_open,
    .read    = seq_read,
    .llseek  = seq_lseek,
    .release = single_release,
};



static struct file_operations proc_pci_fops = {
    .owner   = THIS_MODULE, 
    .open    = proc_pci_open,
    .read    = seq_read,
    .llseek  = seq_lseek,
    .release = single_release,
};


int 
pisces_enclave_create(struct pisces_image * img) 
{

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

    mutex_init(&(enclave->op_lock));

    enclave->state        = ENCLAVE_LOADED;

    enclave->kern_path    = img->kern_path;
    enclave->initrd_path  = img->initrd_path;
    enclave->kern_cmdline = img->cmd_line;

    enclave->id           = enclave_idx;
    
    INIT_LIST_HEAD(&(enclave->memdesc_list));

    init_enclave_fs(enclave);
    init_enclave_pci(enclave);

    enclave->dev          = MKDEV(pisces_major_num, enclave_idx);
    enclave->memdesc_num  = 0;

    cpumask_clear(&(enclave->assigned_cpus));
    enclave->num_cpus     = 0;

    cdev_init(&(enclave->cdev), &enclave_fops);

    enclave->cdev.owner   = THIS_MODULE;
    enclave->cdev.ops     = &enclave_fops;

    

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
	struct proc_dir_entry * pci_entry = NULL;

	memset(name, 0, 128);
	snprintf(name, 128, "enclave-%d", enclave->id);
	
	enclave->proc_dir = proc_mkdir(name, pisces_proc_dir);

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0)
	mem_entry = create_proc_entry("memory", 0444, enclave->proc_dir);
	if (mem_entry) {
	    mem_entry->proc_fops = &proc_mem_fops;
	    mem_entry->data      = enclave;
	}
	
	cpu_entry = create_proc_entry("cpus",   0444, enclave->proc_dir);
	if (cpu_entry) {
	    cpu_entry->proc_fops = &proc_cpu_fops;
	    cpu_entry->data      = enclave;
	}

	pci_entry = create_proc_entry("pci",   0444, enclave->proc_dir);
	if (pci_entry) {
	    pci_entry->proc_fops = &proc_pci_fops;
	    pci_entry->data      = enclave;
	}
#else
	mem_entry = proc_create_data("memory",  0444, enclave->proc_dir, &proc_mem_fops, enclave);
	cpu_entry = proc_create_data("cpus",    0444, enclave->proc_dir, &proc_cpu_fops, enclave);
	pci_entry = proc_create_data("pci",     0444, enclave->proc_dir, &proc_pci_fops, enclave);

#endif


    }

    printk("Enclave created at /dev/pisces-enclave%d\n", MINOR(enclave->dev));

    return enclave_idx;
}


static int 
pisces_enclave_launch(struct pisces_enclave * enclave) 
{

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



int
pisces_enclave_free(struct pisces_enclave * enclave) 
{

    /* Free Enclave device file */
    device_destroy(pisces_class, enclave->dev);
    cdev_del(&(enclave->cdev));

    /* Wait for closure of any active connections, and prevent future ones */
    pisces_ctrl_deinit(enclave);


    /* Free proc entries */
    {
	char name[128];
	
	memset(name, 0, 128);
	snprintf(name, 128, "enclave-%d", enclave->id);

	remove_proc_entry("memory", enclave->proc_dir);
	remove_proc_entry("cpus",   enclave->proc_dir);
	remove_proc_entry("pci",    enclave->proc_dir);
	remove_proc_entry(name,     pisces_proc_dir);
    }

    free_enclave_index(enclave->id);

    deinit_enclave_fs(enclave);
    deinit_enclave_pci(enclave);
    
    /* Remove Memory descriptors */
    {
	struct enclave_mem_block * memdesc = NULL;
	struct enclave_mem_block * tmp     = NULL;

	list_for_each_entry_safe(memdesc, tmp, &(enclave->memdesc_list), node) {
	    list_del(&(memdesc->node));
	    kfree(memdesc);
	}
    }

    kfree(enclave);

    return 0;
}



int 
pisces_enclave_add_cpu(struct pisces_enclave * enclave, 
		       u32                     cpu_id) 
{

    if (cpumask_test_and_set_cpu(cpu_id, &(enclave->assigned_cpus))) {
	// CPU already present
	printk(KERN_ERR "Error tried to add an already present CPU (%d) to enclave %d\n", cpu_id, enclave->id);
	return -1;
    }

    enclave->num_cpus++;

    return 0;
}

int 
pisces_enclave_add_mem(struct pisces_enclave * enclave, 
		       u64                     base_addr, 
		       u32                     pages) 
{
    struct enclave_mem_block * memdesc = kmalloc(sizeof(struct enclave_mem_block), GFP_KERNEL);
    struct enclave_mem_block * iter    = NULL;

    if (IS_ERR(memdesc)) {
	printk(KERN_ERR "Could not allocate memory descriptor\n");
	return -1;
    }

    memdesc->base_addr = base_addr;
    memdesc->pages     = pages;

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

