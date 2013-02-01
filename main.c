#include<linux/kernel.h>
#include<linux/module.h>

#include<linux/fs.h>    /* device file */
#include<linux/types.h>    /* dev_t */
#include<linux/kdev_t.h>    /* MAJOR MINOR MKDEV */
#include<linux/device.h>    /* udev */
#include<linux/cdev.h>    /* cdev_init cdev_add */
#include<asm/uaccess.h> /* get_user and put_user */

#include<linux/init.h>
#include<linux/smp.h>
#include<linux/sched.h>
#include<linux/percpu.h>
#include<linux/bootmem.h>
#include<linux/tboot.h>
#include<linux/gfp.h>
#include<linux/cpuidle.h>

#include<asm/apic.h>
#include<asm/desc.h>
#include<asm/realmode.h>
#include<asm/idle.h>
#include<asm/cpu.h>

#include"inc/gemini.h"
#include"inc/gemini_dev.h"      /* device file ioctls*/

#define AUTHOR "Jiannan Ouyang <ouyang@cs.pitt.edu>"
#define DESC "Gemini: native os consolidation"


//module_param(test, int, S_IRUGO);

static dev_t dev_num; // <major , minor> 
static struct class *cl; // <major , minor> 
static struct cdev c_dev;  
//static int device_major = 0;
static int ref_count = 0;
static char msg_buffer[500];
static char *msg_ptr;

static int device_open(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "Open\n");
    if (ref_count)
        return -EBUSY;
    ref_count++;
    msg_ptr = msg_buffer;

    try_module_get(THIS_MODULE);
    return 0;
}
static int device_release(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "Release\n");
    ref_count--;

    module_put(THIS_MODULE);
    return 0;
}
static ssize_t device_read( 
        struct file *file,
        char __user *buffer,
        size_t length,
        loff_t *offset)
{
    printk(KERN_INFO "Read\n");
    return 0;
    /*
    int bytes_read = 0;

    if (*msg_ptr == 0) return 0;

    while (length && *msg_ptr) {
        put_user(*(msg_ptr++), buffer++);
        length--;
        bytes_read++;
    }
    return bytes_read;
    */
}
static ssize_t device_write(
        struct file *file,
        const char __user *buffer,
        size_t length,
        loff_t *offset)
{
    printk(KERN_INFO "Write\n");
    return 0;
}
static long device_ioctl(
        struct file *file,
        unsigned int ioctl_num,
        unsigned long ioctl_param)
{
    switch (ioctl_num) {
        case G_IOCTL_PING:
            printk(KERN_INFO "GEMINI: ioctl <ping> received.\n");
            break;
    }
    return 0;
}
static struct file_operations fops = {
    .owner = THIS_MODULE,
    .read = device_read,
    .write = device_write,
    .unlocked_ioctl = device_ioctl,
    .open = device_open,
    .release = device_release
};

// return device major number, -1 if failed
static int device_init(void)
{
    if (alloc_chrdev_region(&dev_num, 0, 1, "gemini") < 0)
    {
            return -1;
    }
    printk(KERN_INFO "<Major, Minor>: <%d, %d>\n", MAJOR(dev_num), MINOR(dev_num));

    if ((cl = class_create(THIS_MODULE, "gemini")) == NULL) {
        unregister_chrdev_region(dev_num, 1);
        return -1;
    }
    
    if (device_create(cl, NULL, dev_num, NULL, "gemini") == NULL)
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

static void device_exit(void)
{
    cdev_del(&c_dev);
    device_destroy(cl, dev_num);
    class_destroy(cl);
    unregister_chrdev_region(dev_num, 1);
}

void __init start_secondary(void)
{
    printk(KERN_INFO "GEMINI: jumped back\n");
    native_halt();
}

static int kick_offline_cpu(int cpu) 
{
    int ret = 0;
    int apicid = apic->cpu_present_to_apicid(1);
    unsigned long start_ip = real_mode_header->trampoline_start;
    /*
    struct task_struct *idle;

    idle = (struct task_struct *)kmalloc(sizeof(struct task_struct), GFP_KERNEL);
    init_idle(idle, cpu);
    if (IS_ERR(idle)) {
        printk(KERN_INFO "GEMINI: idle_thread_get error %p\n", idle);
    }

    idle->thread.sp = (unsigned long) (((struct pt_regs *) (THREAD_SIZE + task_stack_page(idle))) - 1);
    per_cpu(current_task, cpu) = idle;
    clear_tsk_thread_flag(idle, TIF_FORK);
    initial_gs = per_cpu_offset(cpu);
    per_cpu(kernel_stack, cpu) = 
        (unsigned long) task_stack_page(idle) -
        KERNEL_STACK_OFFSET + THREAD_SIZE;
    stack_start = idle->thread.sp;
        */
    //early_gdt_descr.address = (unsigned long)per_cpu(gdt_page, cpu).gdt;
    printk(KERN_INFO "GEMINI: start_ip %lx; target ip: %lx\n", start_ip, (unsigned long)start_secondary);

    initial_code = (unsigned long) start_secondary;

    //ret = wakeup_secondary_cpu_via_init(apicid, gemini_trampoline);
    ret = wakeup_secondary_cpu_via_init(apicid, start_ip);
    return ret;

}

static int gemini_init(void)
{
        int ret = 0;

	printk(KERN_INFO "GEMINI: loaded\n");

        ret = device_init();
        if (ret < 0) {
            printk(KERN_INFO "GEMINI: device file registeration failed\n");
            return -1;
        }

        //ret = kick_offline_cpu(1);   
        //printk(KERN_INFO "GEMINI: smp call return: %d\n", ret);
        
	return 0;
}

static void __exit gemini_exit(void)
{
        device_exit();
	printk(KERN_INFO "GEMINI: unloaded\n");
	return;
}

module_init(gemini_init);
module_exit(gemini_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR(AUTHOR);
MODULE_DESCRIPTION(DESC);
