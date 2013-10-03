#include <linux/types.h>
#include <linux/sched.h>
#include <linux/anon_inodes.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include "pisces.h"
#include "pisces_boot_params.h"
#include "pisces_ctrl.h"
#include "xcall.h"
#include "enclave.h"


#include "pisces_cmds.h"


static int send_cmd(struct pisces_enclave * enclave, struct ctrl_cmd * cmd) {
    struct pisces_ctrl * ctrl = &(enclave->ctrl);
    struct ctrl_cmd_buf * cmd_buf = ctrl->cmd_buf;
    int ret = 0;


    if (cmd_buf->active) {
	printk(KERN_ERR "CMD is active. This should be impossible.\n");
	return -1;
    }

    memcpy(&(cmd_buf->cmd), cmd, sizeof(struct ctrl_cmd));
    memcpy(cmd_buf->cmd.data, cmd->data, cmd->data_len);

    // signal that the command is active
    cmd_buf->active = 1;

    pisces_send_ipi(enclave, 0, PISCES_CTRL_IPI_VECTOR);

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
    struct ctrl_cmd_buf * cmd_buf = ctrl->cmd_buf;
    unsigned long flags;
    int ret = 0;

    spin_lock_irqsave(&ctrl->lock, flags);

    
    // read potential resp data

    cmd_buf->completed = 0;
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
    struct ctrl_cmd_buf * cmd_buf = ctrl->cmd_buf;
    void __user * argp = (void __user *)arg;
    int ret = 0;

    switch (ioctl) {
        case ENCLAVE_CMD_ADD_CPU:
            {
                struct cmd_cpu_add  cmd;
                u64 apicid = 0;
                u32 target_addr = 0;

                if (copy_from_user(&apicid, argp, sizeof(u64))) {
                    printk(KERN_ERR "Error copy_from_user in enclave_add_cpu ioctl\n");
                    return -EFAULT;
                }

		cmd.hdr.cmd = ENCLAVE_CMD_ADD_CPU;
		cmd.hdr.data_len = (sizeof(struct cmd_cpu_add) - sizeof(struct ctrl_cmd));
		cmd.apic_id = apicid;
		
                /* TODO: check if target CPU is reserved for Pisces first */
                printk(KERN_INFO "Send enclave xcall to cpu %llu\n", apicid);
		ret = send_cmd(enclave, (struct ctrl_cmd *)&cmd);


                printk("Reset CPU %llu with start_eip 0x%x", apicid, target_addr);
                //wakeup_secondary_cpu_via_init(apicid, target_addr);

		// read potential resp data    

                break;
            }
	    
	case ENCLAVE_CMD_ADD_MEM: 
	    {
		struct cmd_mem_add cmd;
		
		cmd.hdr.cmd = ENCLAVE_CMD_ADD_MEM;
		cmd.hdr.data_len = (sizeof(struct cmd_mem_add) - sizeof(struct ctrl_cmd));

		ret = send_cmd(enclave, (struct ctrl_cmd *)&cmd);

		break;
	    }
    }


    cmd_buf->completed = 0;
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
    struct ctrl_cmd_buf * cmd_buf = ctrl->cmd_buf;
    unsigned long flags = 0;
    int acquired = 0;
    int ctrl_fd = 0;
    
    if (cmd_buf->ready == 0) {
	printk(KERN_ERR "Cannot connect to unitialized control channel\n");
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
    ctrl->cmd_buf = (struct ctrl_cmd_buf *)__va(boot_params->control_buf_addr);

    return 0;
}

