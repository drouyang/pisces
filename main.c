#include"inc/pisces.h"
#include"inc/loader.h"
#include"inc/pisces_dev.h"      /* device file ioctls*/

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
#define DESC "Pisces: native os consolidation"


extern bootstrap_pgt_t *bootstrap_pgt;

unsigned long mem_base = 0x8000000;
module_param(mem_base, ulong, S_IRWXU);

unsigned long mem_len = 0x8000000;
module_param(mem_len, ulong, S_IRWXU);

unsigned long cpu_id = 1;
module_param(cpu_id, ulong, S_IRWXU);

char *kernel_path = "/home/ouyang/pisces/kitten-1.3.0/vmlwk.bin";
module_param(kernel_path, charp, S_IRWXU);

char *initrd_path = "/home/ouyang/pisces/kitten-1.3.0/arch/x86_64/boot/isoimage/initrd.img";
module_param(initrd_path, charp, S_IRWXU);

char *boot_cmd_line = "console=pisces init_argv=\"one two three\" init_envp=\"a=1 b=2 c=3\"";
module_param(boot_cmd_line, charp, S_IRWXU);

unsigned long mpf_addr = 0xffff8800000fda60;
module_param(mpf_addr, ulong, S_IRWXU);


int wakeup_secondary_cpu_via_init(int, unsigned long);

static dev_t dev_num; // <major , minor> 
static struct class *cl; // <major , minor> 
static struct cdev c_dev;  
static struct pisces_mmap_t pisces_mmap;
static struct boot_params_t *boot_params;
static struct shared_info_t *shared_info;
static char console_buffer[1024*50];
static u64 console_idx = 0;
static struct pisces_mpf_intel *mpf;

void test(void)
{
    printk(KERN_INFO "PISCES: test \n");
}
static void pisces_trampoline(void)
{
    unsigned long pgd_phys = __pa((unsigned long)bootstrap_pgt->level4_pgt);
    unsigned long target = (unsigned long)(boot_params->kernel_addr);
    int magic = PISCES_MAGIC;

    printk(KERN_INFO "PISCES: jump to physical address 0x%lx\n", target);
    __asm__ ( "movq %0, %%rax\n\t"
            "movq %%rax, %%cr3\n\t" //cr3
            "movl %2, %%esi\n\t" // PISCES_MAGIC
            "movq %1, %%rax\n\t" //target
            "jmp *%%rax\n\t" // should never return
            "hlt\n\t"
            : 
            : "r" (pgd_phys), "r" (target), "r" ((unsigned int)boot_params->shared_info_addr | magic)
            : "%rax", "%esi");
}

// use linux trampoline code to init offlined cpu, but hijack and jump to pisces_trampoline
// in pisces_trampoline, setup ident mapping and jump to kernel code
static int kick_offline_cpu(void) 
{
    int ret = 0;
    int apicid = apic->cpu_present_to_apicid(cpu_id);
    unsigned long start_ip = real_mode_header->trampoline_start;

    // gdt for the kernel to access user space memory
    early_gdt_descr.address = (unsigned long)per_cpu(gdt_page, cpu_id).gdt;

    // setup ident mapping for pisces_trampoline
    pgtable_setup_ident(mem_base, mem_len);

    // our pisces_trampoline
    initial_code = (unsigned long) pisces_trampoline;

    printk(KERN_INFO "PISCES: CPU%d (apic_id %d) wakeup CPU%lu (apic_id %d) via INIT\n", smp_processor_id(), apic->cpu_present_to_apicid(smp_processor_id()), cpu_id, apicid);
    ret = wakeup_secondary_cpu_via_init(apicid, start_ip);
    return ret;

}
static void debug(void)
{
        struct mpc_table *mpc = NULL;

	if (!mpf) {
		printk(KERN_INFO "Assuming 1 CPU.\n");
		return;
	}

	printk(KERN_INFO "Intel MultiProcessor Specification v1.%d\n",
	       mpf->specification);
	if (mpf->feature2 & (1<<7)) {
		printk(KERN_INFO "    IMCR and PIC compatibility mode.\n");
	} else {
		printk(KERN_INFO "    Virtual Wire compatibility mode.\n");
	}

	/*
	 * We don't support the default MP configuration.
	 * All supported multi-CPU systems must provide a full MP table.
	 */
	if (mpf->feature1 != 0)
		BUG();
	if (!mpf->physptr)
		BUG();

	/*
	 * Parse the MP configuration
	 */
	mpc = (struct mpc_table *)phys_to_virt(mpf->physptr);
        if (memcmp(mpc->signature, MPC_SIGNATURE, 4)) {
		BUG();
        }
        printk("nice!!");
}
static void start_instance(void)
{
        struct pisces_mmap_t *mmap = &pisces_mmap;

        // 0. setup offlined memory map
        mmap->nr_map = 1;
        mmap->map[0].addr = mem_base;
        mmap->map[0].size = mem_len;

        // 1. load images, setup loader memory layout
        boot_params = setup_memory_layout(mmap);
        strcpy(boot_params->cmd_line, boot_cmd_line);
        shared_info = container_of(boot_params, struct shared_info_t, boot_params);
        
#if 0
        // check and compare with original binary file using xxd
        printk(KERN_INFO "PISCES: boot command line: %s\n", boot_params->cmd_line); 
        printk(KERN_INFO "PISCES: kernel image header 0x%lx: 0x%lx\n", 
                boot_params->kernel_addr, *(unsigned long *)__va(boot_params->kernel_addr));
        printk(KERN_INFO "PISCES: initrd image header 0x%lx: 0x%lx\n", 
                boot_params->initrd_addr, *(unsigned long *)__va(boot_params->initrd_addr));
#endif

        // 2. setup MP tables
        memcpy(&boot_params->mpf, mpf, sizeof(struct pisces_mpf_intel));
        memcpy(&boot_params->mpc, __va(mpf->physptr), sizeof(struct pisces_mpc_table));
        memcpy(&boot_params->mpc, __va(mpf->physptr), boot_params->mpc.length);
        boot_params->mpf.physptr = __pa(&boot_params->mpc);

        // update MP Floating Pointer (mpf) checksum
        {
            unsigned char checksum = 0;
            int len = 16;
            unsigned char *mp = (unsigned char *) &boot_params->mpf;

            boot_params->mpf.checksum = 0;
            while (len--)
                checksum -= *mp++;
            boot_params->mpf.checksum = checksum;
        }

        printk(KERN_INFO "PISCES: MP tables length %d\n", boot_params->mpc.length); 

        // 3. kick offlined cpu
        kick_offline_cpu();

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
                /*
                do {
                    offset = printk(KERN_INFO "%s", start);
                    start += offset+1;
                    while (*start != 0) start--;
                    start++;
                } while (start < console_buffer+console_idx);
                */
                printk(KERN_INFO "===PISCES_GUEST END===\n");
                printk(KERN_INFO "PISCES: %ld char printed out of %lld\n", start-console_buffer, console_idx);
                
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

static void device_exit(void)
{
    cdev_del(&c_dev);
    device_destroy(cl, dev_num);
    class_destroy(cl);
    unregister_chrdev_region(dev_num, 1);
}


static int pisces_init(void)
{
        int ret = 0; 

	printk(KERN_INFO "PISCES: loaded\n");

        mpf = (struct pisces_mpf_intel *)mpf_addr; 


        ret = device_init();
        if (ret < 0) {
            printk(KERN_INFO "PISCES: device file registeration failed\n");
            return -1;
        }

        //debug();
        start_instance();

	return 0;
}

static void pisces_exit(void)
{
        device_exit();
	printk(KERN_INFO "PISCES: unloaded\n");
	return;
}

module_init(pisces_init);
module_exit(pisces_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR(AUTHOR);
MODULE_DESCRIPTION(DESC);
