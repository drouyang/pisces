#include <linux/types.h>
#include <linux/sched.h>
#include <linux/anon_inodes.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include "pisces.h"
#include "pisces_boot_params.h"
#include "pisces_ctrl.h"
#include "ipi.h"
#include "enclave.h"
#include "boot.h"
#include "pisces_cmds.h"


static int send_cmd(struct pisces_enclave * enclave, struct pisces_cmd * cmd) {
    struct pisces_ctrl * ctrl = &(enclave->ctrl);
    struct pisces_cmd_buf * cmd_buf = ctrl->cmd_buf;
    int ret = 0;


    if (cmd_buf->active) {
        printk(KERN_ERR "CMD is active. This should be impossible.\n");
        return -1;
    }

    memcpy(&(cmd_buf->cmd), cmd, sizeof(struct pisces_cmd));
    memcpy(cmd_buf->cmd.data, cmd->data, cmd->data_len);

    // signal that the command is active
    cmd_buf->active = 1;
    cmd_buf->completed = 0;
    cmd_buf->in_progress = 0;

    pisces_send_ipi(enclave, cmd_buf->enclave_cpu, cmd_buf->enclave_vector);

    while (cmd_buf->completed == 0) {
        schedule();
    }

    // read response
    ret = cmd_buf->resp.status;


    return ret;
}






static ssize_t
ctrl_read(struct file * filp, char __user * buffer, size_t length, loff_t * offset ) {
    struct pisces_enclave * enclave = filp->private_data;
    struct pisces_ctrl * ctrl = &(enclave->ctrl);
    struct pisces_cmd_buf * cmd_buf = ctrl->cmd_buf;
    unsigned long flags;
    int ret = 0;

    spin_lock_irqsave(&ctrl->lock, flags);


    // read potential resp data

    cmd_buf->completed = 0;
    cmd_buf->in_progress = 0;
    cmd_buf->active = 0;

    spin_unlock_irqrestore(&ctrl->lock, flags);

    return ret;
}

// Allow ctrl server to write in a raw command
static ssize_t ctrl_write(struct file * filp, const char __user * buffer, size_t length, loff_t * offset) {
    //   struct pisces_enclave * enclave = filp->private_data;
    //   struct pisces_ctrl * ctrl = &(enclave->ctrl);
    return 0;
}



// Allow high level control commands over ioctl
static long ctrl_ioctl(struct file * filp, unsigned int ioctl, unsigned long arg) {
    struct pisces_enclave * enclave = filp->private_data;
    struct pisces_ctrl * ctrl = &(enclave->ctrl);
    struct pisces_cmd_buf * cmd_buf = ctrl->cmd_buf;
    void __user * argp = (void __user *)arg;
    int ret = 0;

    switch (ioctl) {
        case ENCLAVE_IOCTL_ADD_CPU:
            {
                struct cmd_cpu_add  cmd;
                u64 cpu_id = (u64)arg;

		memset(&cmd, 0, sizeof(struct cmd_cpu_add));

		if (pisces_enclave_add_cpu(enclave, cpu_id) != 0) {
		    printk(KERN_ERR "Error adding CPU to enclave %d\n", enclave->id);
		    return -1;
		}

                cmd.hdr.cmd = ENCLAVE_CMD_ADD_CPU;
                cmd.hdr.data_len = (sizeof(struct cmd_cpu_add) - sizeof(struct pisces_cmd));
                cmd.phys_cpu_id = cpu_id;

                /* Setup Linux trampoline to jump to enclave trampoline */
                trampoline_lock();
                setup_linux_trampoline_pgd(enclave->bootmem_addr_pa);
                setup_linux_trampoline_target(enclave->bootmem_addr_pa);
                ret = send_cmd(enclave, (struct pisces_cmd *)&cmd);
                trampoline_unlock();

		if (ret != 0) {
		    // remove CPU from enclave
		    return -1;
		}

                break;
            }

        case ENCLAVE_IOCTL_ADD_MEM:
            {
                struct cmd_mem_add cmd;
                struct memory_range reg;

                memset(&cmd, 0, sizeof(struct cmd_mem_add));
                memset(&reg, 0, sizeof(struct memory_range));

                cmd.hdr.cmd = ENCLAVE_CMD_ADD_MEM;
                cmd.hdr.data_len = (sizeof(struct cmd_mem_add) - sizeof(struct pisces_cmd));

                if (copy_from_user(&reg, argp, sizeof(struct memory_range))) {
                    printk(KERN_ERR "Could not copy memory region from user space\n");
                    return -EFAULT;
                }

		if (pisces_enclave_add_mem(enclave, reg.base_addr, reg.pages) != 0) {
		    printk(KERN_ERR "Error adding memory descriptor to enclave %d\n", enclave->id);
		    return -1;
		}

                cmd.phys_addr = reg.base_addr;
                cmd.size = reg.pages * PAGE_SIZE_4KB;

                ret = send_cmd(enclave, (struct pisces_cmd *)&cmd);

		if (ret != 0) {
		    printk(KERN_ERR "Error adding memory to enclave %d\n", enclave->id);
		    // remove memory from enclave
		    return -1;
		}


                break;
            }

	case ENCLAVE_IOCTL_TEST_LCALL:
	    {
		struct pisces_cmd cmd;

		memset(&cmd, 0, sizeof(struct pisces_cmd));

		cmd.cmd = ENCLAVE_CMD_TEST_LCALL;
		cmd.data_len = 0;

		ret = send_cmd(enclave, &cmd);

		printk("Sent LCALL test CMD\n");
		break;
	    }

    }


    cmd_buf->completed = 0;
    cmd_buf->in_progress = 0;
    cmd_buf->active = 0;


    return 0;
}





static int ctrl_release(struct inode * i, struct file * filp) {
    struct pisces_enclave * enclave = filp->private_data;
    struct pisces_ctrl * ctrl = &(enclave->ctrl);
    unsigned long flags;

    spin_lock_irqsave(&(ctrl->lock), flags);
    ctrl->connected = 0;
    spin_unlock_irqrestore(&(ctrl->lock), flags);

    return 0;
}


static struct file_operations ctrl_fops = {
    .write    = ctrl_write,
    .read     = ctrl_read,
    .unlocked_ioctl    = ctrl_ioctl,
    .release  = ctrl_release,
};



int pisces_ctrl_connect(struct pisces_enclave * enclave) {
    struct pisces_ctrl * ctrl = &(enclave->ctrl);
    struct pisces_cmd_buf * cmd_buf = ctrl->cmd_buf;
    unsigned long flags = 0;
    int acquired = 0;
    int ctrl_fd = 0;

    if (cmd_buf->ready == 0) {
        printk(KERN_ERR "Cannot connect to uninitialized control channel\n");
        return -1;
    }

    spin_lock_irqsave(&ctrl->lock, flags);
    if (ctrl->connected == 0) {
        ctrl->connected = 1;
        acquired = 1;
    }
    spin_unlock_irqrestore(&ctrl->lock, flags);

    if (acquired == 0) {
        printk(KERN_ERR "Ctrl already connected\n");
        return -1;
    }

    ctrl_fd = anon_inode_getfd("enclave-ctrl", &ctrl_fops, enclave, O_RDWR);

    if (ctrl_fd < 0) {
        printk(KERN_ERR "Error creating Ctrl inode\n");
        return ctrl_fd;
    }

    return ctrl_fd;
}



int
pisces_ctrl_init( struct pisces_enclave * enclave) {
    struct pisces_ctrl * ctrl = &(enclave->ctrl);
    struct pisces_boot_params * boot_params = NULL;


    init_waitqueue_head(&ctrl->waitq);
    spin_lock_init(&ctrl->lock);
    ctrl->connected = 0;

    boot_params = __va(enclave->bootmem_addr_pa);
    ctrl->cmd_buf = (struct pisces_cmd_buf *)__va(boot_params->control_buf_addr);

    return 0;
}

