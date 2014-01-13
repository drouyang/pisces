#include <linux/types.h>
#include <linux/sched.h>
#include <linux/anon_inodes.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/slab.h>


#include "pisces.h"
#include "pisces_boot_params.h"
#include "pisces_ctrl.h"
#include "ipi.h"
#include "enclave.h"
#include "boot.h"
#include "pisces_cmds.h"
#include "pisces_xbuf.h"
#include "pisces_pci.h"

#include "v3_console.h"


static ssize_t
ctrl_read(struct file * filp, char __user * buffer, size_t length, loff_t * offset ) {
    struct pisces_enclave * enclave = filp->private_data;
    struct pisces_ctrl * ctrl = &(enclave->ctrl);
    unsigned long flags;
    int ret = 0;

    spin_lock_irqsave(&ctrl->lock, flags);


    // read potential resp data

    spin_unlock_irqrestore(&ctrl->lock, flags);

    return ret;
}

// Allow ctrl server to write in a raw command
static ssize_t ctrl_write(struct file * filp, const char __user * buffer, size_t length, loff_t * offset) {
    //  struct pisces_enclave * enclave = filp->private_data;


    return 0;
}



// Allow high level control commands over ioctl
static long ctrl_ioctl(struct file * filp, unsigned int ioctl, unsigned long arg) {
    struct pisces_enclave * enclave = filp->private_data;
    struct pisces_ctrl * ctrl = &(enclave->ctrl);
    struct pisces_xbuf_desc * xbuf_desc = ctrl->xbuf_desc;
    struct pisces_resp * resp = NULL;
    u32 resp_len = 0;
    void __user * argp = (void __user *)arg;
    int ret = 0;

    printk("CTRL IOCTL (%d)\n", ioctl);

    switch (ioctl) {
        case ENCLAVE_CMD_ADD_CPU:
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
                cmd.apic_id = apic->cpu_present_to_apicid(cpu_id);


                printk("Adding CPU %llu (APIC %llu)\n", cpu_id, cmd.apic_id);

                /* Setup Linux trampoline to jump to enclave trampoline */
                trampoline_lock();
                set_linux_trampoline(enclave);

                printk("Sending Command\n");

                ret = pisces_xbuf_sync_send(xbuf_desc, (u8 *)&cmd, sizeof(struct cmd_cpu_add),  (u8 **)&resp, &resp_len);

		kfree(resp);

                restore_linux_trampoline(enclave);
                trampoline_unlock();

                printk("\tDone\n");

                if (ret != 0) {
                    // remove CPU from enclave
                    return -1;
                }

                break;
            }

        case ENCLAVE_CMD_ADD_MEM:
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

                ret = pisces_xbuf_sync_send(xbuf_desc, (u8 *)&cmd, sizeof(struct cmd_mem_add),  (u8 **)&resp, &resp_len);

		kfree(resp);

                if (ret != 0) {
                    printk(KERN_ERR "Error adding memory to enclave %d\n", enclave->id);
                    // remove memory from enclave
                    return -1;
                }


                break;
            }

        case ENCLAVE_CMD_TEST_LCALL:
            {
                struct pisces_cmd cmd;

                memset(&cmd, 0, sizeof(struct pisces_cmd));

                cmd.cmd = ENCLAVE_CMD_TEST_LCALL;
                cmd.data_len = 0;


                ret = pisces_xbuf_sync_send(xbuf_desc, (u8 *)&cmd, sizeof(struct cmd_mem_add),  (u8 **)&resp, &resp_len);

		kfree(resp);

                printk("Sent LCALL test CMD\n");
                break;
            }

	case ENCLAVE_CMD_ADD_V3_PCI:
	    {
		struct cmd_add_pci_dev cmd;
		struct cmd_add_pci_dev * cmd_resp;
                struct pisces_assigned_dev * assigned_dev; 

		printk("Adding V3 PCI device\n");

		memset(&cmd, 0, sizeof(struct cmd_add_pci_dev));

		cmd.hdr.cmd = ENCLAVE_CMD_ADD_V3_PCI;
		cmd.hdr.data_len = (sizeof(struct cmd_add_pci_dev) - sizeof(struct pisces_cmd));

		if (copy_from_user(&(cmd.device), argp, sizeof(struct pisces_pci_dev))) {
		    printk(KERN_ERR "Could not copy pci device structure from user space\n");
		    return -EFAULT;
		}

		printk("Init an offlined PCI device\n");
		assigned_dev = pisces_pci_dev_init(&cmd.device);

                if (assigned_dev == NULL) {
                    printk(KERN_ERR "Error init pci device\n");
		    return -1;
                }

                assigned_dev->enclave = enclave;

		printk(" Notifying enclave\n");
		ret = pisces_xbuf_sync_send(xbuf_desc, (u8 *)&cmd, sizeof(struct cmd_add_pci_dev), (u8 **)&cmd_resp, &resp_len);

                if (cmd_resp->device_ipi_vector <= 0) {
                    printk(KERN_ERR "Error negtive device_ipi_vector\n");
		    return -1;
                }
                assigned_dev->device_ipi_vector = cmd_resp->device_ipi_vector;

		kfree(cmd_resp);
		
		if (ret != 0) {
		    printk(KERN_ERR "Error adding PCI device to Enclave %d\n", enclave->id);
		    return -1;
		}

		break;
	    }

        case ENCLAVE_CMD_CREATE_VM:
            {
                struct cmd_create_vm cmd;

                memset(&cmd, 0, sizeof(struct cmd_create_vm));

                cmd.hdr.cmd = ENCLAVE_CMD_CREATE_VM;
                cmd.hdr.data_len = (sizeof(struct cmd_create_vm) - sizeof(struct pisces_cmd));

                if (copy_from_user(&(cmd.path), argp, sizeof(struct vm_path))) {
                    printk(KERN_ERR "Could not copy vm path from user space\n");
                    return -EFAULT;
                }

                ret = pisces_xbuf_sync_send(xbuf_desc, (u8 *)&cmd, sizeof(struct cmd_create_vm),  (u8 **)&resp, &resp_len);

		kfree(resp);

                if (ret != 0) {
                    printk("Error creating VM %s (%s)\n", cmd.path.vm_name, cmd.path.file_name);
                    return -1;
                }

                break;
            }
        case ENCLAVE_CMD_LAUNCH_VM: 
            {
                struct cmd_launch_vm cmd;

                memset(&cmd, 0, sizeof(struct cmd_launch_vm));

                cmd.hdr.cmd = ENCLAVE_CMD_LAUNCH_VM;
                cmd.hdr.data_len = (sizeof(struct cmd_launch_vm) - sizeof(struct pisces_cmd));

                cmd.vm_id = arg;

                ret = pisces_xbuf_sync_send(xbuf_desc, (u8 *)&cmd, sizeof(struct cmd_launch_vm),  (u8 **)&resp, &resp_len);
		kfree(resp);

                if (ret != 0) {
                    printk("Error Launch VM %d\n", cmd.vm_id);
                    return -1;
                }

                break;
            }
	case ENCLAVE_CMD_VM_CONS_CONNECT:
	    {
		struct cmd_vm_cons_connect cmd;
		uintptr_t cons_pa = 0;
		u32 vm_id = arg;

		memset(&cmd, 0, sizeof(struct cmd_vm_cons_connect));

		cmd.hdr.cmd = ENCLAVE_CMD_VM_CONS_CONNECT;
		cmd.hdr.data_len = (sizeof(struct cmd_vm_cons_connect) - sizeof(struct pisces_cmd));

		cmd.vm_id = vm_id;

		printk("Connecting to VM Console in Kitten (&resp=%p)\n", &resp);

		ret = pisces_xbuf_sync_send(xbuf_desc, 
					    (u8 *)&cmd, sizeof(struct cmd_vm_cons_connect), 
					    (u8 **)&resp, &resp_len);
		


		if ((ret != 0) || (resp == NULL)) {
		    printk("Error connecting to VM (%d) console\n", cmd.vm_id);
		    return -1;
		}

		printk("Getting Console PAddr (resp=%p)\n", resp);

		cons_pa = (uintptr_t)(resp->status);


		printk("Console found at %p\n", (void *)cons_pa);

		kfree(resp);

		if (cons_pa == 0) {
		    printk("Could not acquire console ring buffer\n");
		    return -1;
		}

		return v3_console_connect(enclave, vm_id, cons_pa);
		break;
	    }

	case ENCLAVE_CMD_VM_CONS_KEYCODE:
	    {
		struct cmd_vm_cons_keycode cmd;

		memset(&cmd, 0, sizeof(struct cmd_vm_cons_keycode));

		cmd.hdr.cmd = ENCLAVE_CMD_VM_CONS_KEYCODE;
		cmd.hdr.data_len = (sizeof(struct cmd_vm_cons_keycode) - sizeof(struct pisces_cmd));
		cmd.vm_id = 0;
		cmd.scan_code = (u8)arg;

		printk("Directly Sending Scan_Code %x\n", cmd.scan_code);

		pisces_xbuf_send(xbuf_desc, (u8 *)&cmd, sizeof(struct cmd_vm_cons_keycode));
    
		break;

	    }
	case ENCLAVE_CMD_VM_DBG:
	    {
		printk("Sending Debug IPI to palacios\n");
		pisces_send_ipi(enclave, 0, 169);
		break;
	    }
	default: 
	    {
		printk(KERN_ERR "Unknown Enclave IOCTL (%d)\n", ioctl);
		return -1;
	    }
		   

    }

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
    unsigned long flags = 0;
    int acquired = 0;
    int ctrl_fd = 0;


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
    
    ctrl->xbuf_desc = pisces_xbuf_client_init(enclave, (uintptr_t)__va(boot_params->control_buf_addr), 0, 0);
    
    

    return 0;
}

 
