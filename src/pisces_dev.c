#include<linux/kernel.h>
#include<linux/module.h>
#include<linux/fs.h>    /* device file */
#include<linux/types.h>    /* dev_t */
#include<linux/kdev_t.h>    /* MAJOR MINOR MKDEV */
#include<linux/device.h>    /* udev */
#include<linux/cdev.h>    /* cdev_init cdev_add */
#include<linux/moduleparam.h>    /* module_param */
#include<linux/stat.h>    /* perms */
#include<asm/uaccess.h>

#include"pisces_dev.h"      /* device file ioctls*/
#include"pisces_mod.h"      
#include"pisces_loader.h"      
#include"pisces.h"      
#include"domain_xcall.h"      

struct cdev c_dev;  
static dev_t dev_num; // <major , minor> 
static struct class *cl; // <major , minor> 
static char console_buffer[1024*50];
static u64 console_idx = 0;

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

static ssize_t device_read( 
        struct file *file,
        char __user *buffer,
        size_t length,
        loff_t *offset)
{
    int count = 0;
    struct pisces_cons_t *console = &shared_info->console;
    u64 *cons = &console->out_cons;
    u64 *prod = &console->out_prod;

    while(!(*prod == *cons)) {//not empty
        console_buffer[console_idx++] = console->out[*cons];
        *cons = (*cons + 1) % PISCES_CONSOLE_SIZE_OUT;
    }

    if (*offset >= console_idx)
        return 0;

    count = (length < console_idx-*offset)? length : console_idx-*offset;
    if (copy_to_user(buffer, console_buffer+*offset, count))
        return -EFAULT;
    *offset += count;
    return count;
}
static ssize_t device_write(
        struct file *file,
        const char __user *buffer,
        size_t length,
        loff_t *offset)
{
    printk(KERN_INFO "Write\n");
    return length;
}
static long device_ioctl(
        struct file *file,
        unsigned int ioctl_num,
        unsigned long ioctl_param)
{
    long ret;
    switch (ioctl_num) {
        case G_IOCTL_PING:
            printk(KERN_INFO "PISCES: mem_base 0x%lx, mem_len 0x%lx, cpuid %lu, kernel_path '%s'\n", 
                    mem_base, mem_len, cpu_id, kernel_path);
            break;

        case G_IOCTL_PREPARE_SECONDARY:

            printk(KERN_INFO "PISCES: setup bootstrap page table for [0x%lx, 0x%lx)\n", 
                    mem_base, mem_base+mem_len);
            pgtable_setup_ident(mem_base, mem_len);

        case G_IOCTL_LOAD_IMAGE:

            ret = load_image(kernel_path, mem_base);
            printk(KERN_INFO "PISCES: load %lu bytes (%lu KB) to physicall address 0x%lx from %s\n",
                    ret, ret/1024, mem_base, kernel_path);

            break;

        case G_IOCTL_START_SECONDARY:
            printk(KERN_INFO "PISCES: start secondary cpu %ld\n", cpu_id);
            kick_offline_cpu();

            break;

        case G_IOCTL_PRINT_IMAGE:
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
            /*
        case G_IOCTL_READ_CONSOLE_BUFFER:
            {
                struct pisces_cons_t *console = &shared_info->console;
                u64 *cons = &console->out_cons;
                u64 *prod = &console->out_prod;
                int offset;
                char *start;

                pisces_spin_lock(&console->lock_out);
                
                while(!(*prod == *cons)) {//not empty
                    console_buffer[console_idx++] = console->out[*cons];
                    *cons = (*cons + 1) % PISCES_CONSOLE_SIZE_OUT;
                }

                pisces_spin_unlock(&console->lock_out);


                start = console_buffer;
                offset = 0;
                printk(KERN_INFO "===PISCES_GUEST START===\n");
                offset = printk(KERN_INFO "%s", start);
                printk(KERN_INFO "hello");
                printk(KERN_INFO "%s", start+400);
                printk(KERN_INFO "===PISCES_GUEST END===\n");
                printk(KERN_INFO "PISCES: %ld char printed out of %lld\n", start-console_buffer, console_idx);
                
                break;
            
            }*/


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
int device_init(void)
{
    if (alloc_chrdev_region(&dev_num, 0, 1, "pisces") < 0)
    {
            return -1;
    }
    //printk(KERN_INFO "<Major, Minor>: <%d, %d>\n", MAJOR(dev_num), MINOR(dev_num));

    if ((cl = class_create(THIS_MODULE, "pisces")) == NULL) {
        unregister_chrdev_region(dev_num, 1);
        return -1;
    }
    
    if (device_create(cl, NULL, dev_num, NULL, "pisces") == NULL)
    {
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

void device_exit(void)
{
    cdev_del(&c_dev);
    device_destroy(cl, dev_num);
    class_destroy(cl);
    unregister_chrdev_region(dev_num, 1);
}
