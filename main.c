#include"inc/gemini.h"
#include"inc/loader.h"
#include"inc/gemini_dev.h"      /* device file ioctls*/

#include<linux/kernel.h>
#include<linux/module.h>

#include<linux/fs.h>    /* device file */
#include<linux/types.h>    /* dev_t */
#include<linux/kdev_t.h>    /* MAJOR MINOR MKDEV */
#include<linux/device.h>    /* udev */
#include<linux/cdev.h>    /* cdev_init cdev_add */
#include<linux/moduleparam.h>    /* module_param */
#include<linux/stat.h>    /* perms */

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


#define AUTHOR "Jiannan Ouyang <ouyang@cs.pitt.edu>"
#define DESC "Gemini: native os consolidation"


extern bootstrap_pgt_t *bootstrap_pgt;

unsigned long mem_base = 0x8000000;
module_param(mem_base, ulong, S_IRWXU);

unsigned long mem_len = 0x8000000;
module_param(mem_len, ulong, S_IRWXU);

unsigned long cpu_id = 1;
module_param(cpu_id, ulong, S_IRWXU);

char *path = "/home/ouyang/gemini/kitten-1.3.0/vmlwk.bin";
module_param(path, charp, S_IRWXU);

char *initrd_path = "/home/ouyang/gemini/kitten-1.3.0/arch/x86_64/boot/isoimage/initrd.img";
module_param(initrd_path, charp, S_IRWXU);

char *boot_cmd_line = "console=serial init_argv=\"one two three\" init_envp=\"a=1 b=2 c=3\"";
module_param(boot_cmd_line, charp, S_IRWXU);


static dev_t dev_num; // <major , minor> 
static struct class *cl; // <major , minor> 
static struct cdev c_dev;  

void test(void)
{
    printk(KERN_INFO "GEMINI: test \n");
}
static void hooker(void)
{
    unsigned long pgd_phys = __pa((unsigned long)bootstrap_pgt->level4_pgt);
    unsigned long target = (unsigned long)(mem_base);

    printk(KERN_INFO "GEMINI: enter hooker, load cr3 %lx\n", pgd_phys);
    __asm__ ( "movq %0, %%rax\n\t"
            "movq %%rax, %%cr3\n\t"
            "movq $1f, %%rax\n\t"
            "jmp *%%rax\n\t"
            "1:\n\t"
            "movq %1, %%rax\n\t"
            "movq $0x5a5a, %%rbx\n\t"
            "movq (%%rax), %%rcx\n\t"
            "jmp *%%rax\n\t"
            "hlt\n\t"
            : 
            : "r" (pgd_phys), "r" (target)
            : "%rax", "%rbx", "%rcx");
}

static int kick_offline_cpu(void) 
{
    int ret = 0;
    int apicid = apic->cpu_present_to_apicid(cpu_id);
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

    //printk(KERN_INFO "GEMINI: start_ip %lx; target ip: %lx\n", start_ip, (unsigned long)hooker);

    // gdt for the kernel to access user space memory
    early_gdt_descr.address = (unsigned long)per_cpu(gdt_page, cpu_id).gdt;
    // our hooker
    initial_code = (unsigned long) hooker;

    printk(KERN_INFO "GEMINI: CPU%d wakeup CPU%lu(%d) via INIT\n", smp_processor_id(), cpu_id, apicid);
    ret = wakeup_secondary_cpu_via_init(apicid, start_ip);
    return ret;

}

static int device_open(struct inode *inode, struct file *file)
{
    //printk(KERN_INFO "Open\n");

    try_module_get(THIS_MODULE);
    return 0;
}
static int device_release(struct inode *inode, struct file *file)
{
    //printk(KERN_INFO "Release\n");

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
            printk(KERN_INFO "GEMINI: mem_base 0x%lx, mem_len 0x%lx, cpuid %lu, path '%s'\n", 
                    mem_base, mem_len, cpu_id, path);
            break;

        case G_IOCTL_PREPARE_SECONDARY:

            printk(KERN_INFO "GEMINI: setup bootstrap page table for [0x%lx, 0x%lx)\n", 
                    mem_base, mem_base+mem_len);
            pgtable_setup_ident(mem_base, mem_len);

        case G_IOCTL_LOAD_IMAGE:

            ret = load_image(path, mem_base);
            printk(KERN_INFO "GEMINI: load %lu bytes (%lu KB) to physicall address 0x%lx from %s\n",
                    ret, ret/1024, mem_base, path);

            break;

        case G_IOCTL_START_SECONDARY:
            printk(KERN_INFO "GEMINI: start secondary cpu %ld\n", cpu_id);
            kick_offline_cpu();

            break;

        case G_IOCTL_PRINT_IMAGE:
            {
                long *p = (long *)__va(mem_base);
                //long *p = (long *)0x8000000;
                int t=10;
                printk(KERN_INFO "GEMINI: physicall address 0x%lx\n", mem_base);
                while (t>0) {
                    printk(KERN_INFO "%p\t", (void *)*p);
                    p++;
                    t--;
                }

                break;
            
            }


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
static int device_init(void)
{
    if (alloc_chrdev_region(&dev_num, 0, 1, "gemini") < 0)
    {
            return -1;
    }
    //printk(KERN_INFO "<Major, Minor>: <%d, %d>\n", MAJOR(dev_num), MINOR(dev_num));

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


static int gemini_init(void)
{
        int ret = 0;

	printk(KERN_INFO "GEMINI: loaded\n");

        ret = device_init();
        if (ret < 0) {
            printk(KERN_INFO "GEMINI: device file registeration failed\n");
            return -1;
        }

#if 1
        // load and start sibling OS without ioctl
        pgtable_setup_ident(mem_base, mem_len);
        load_image(path, mem_base);
            {
                long *p = (long *)__va(mem_base);
                //long *p = (long *)0x8000000;
                int t=10;
                printk(KERN_INFO "GEMINI: pa 0x%lx, va 0x%lx\n", mem_base, (unsigned long)p);
                while (t>0) {
                    printk(KERN_INFO "%p\t", (void *)*p);
                    p++;
                    t--;
                }
            }
        kick_offline_cpu();
#endif

	return 0;
}

static void gemini_exit(void)
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
