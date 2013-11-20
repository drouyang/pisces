/*
 * Device Assignment
 * (c) Jiannan Ouyang, 2013 (ouyang@cs.pitt.edu)
 */

#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/list.h>
#include <linux/pci.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/iommu.h>
#include <linux/pci.h>

#include "pisces_pci.h"

/*
#define PCI_BUS_MAX  7
#define PCI_DEV_MAX 32
#define PCI_FN_MAX   7

#define PCI_DEVICE 0x0
#define PCI_PCI_BRIDGE 0x1
#define PCI_CARDBUS_BRIDGE 0x2
*/

#define PCI_HDR_SIZE 256

static LIST_HEAD(assigned_device_list);
static DEFINE_SPINLOCK(assigned_device_list_lock);

static struct pisces_assigned_dev * find_dev_by_name(char * name) {
    struct pisces_assigned_dev * dev = NULL;
    unsigned long flags;

    spin_lock_irqsave(&assigned_device_list_lock, flags);
    list_for_each_entry(dev, &assigned_device_list, dev_node) {
        if (strncmp(dev->name, name, 128) == 0) {
            spin_unlock_irqrestore(&assigned_device_list_lock, flags);
            return dev;
        }
    }
    spin_unlock_irqrestore(&assigned_device_list_lock, flags);

    return NULL;
}

/*
 * Reserve a PCI device
 *  - ensures existence, non-bridge type, IOMMU enabled
 *  - enable device and assigned_device_list
 */
int pisces_device_assign(struct pisces_host_pci_bdf * bdf)
{
    struct pisces_assigned_dev * assigned_dev = NULL;
    struct pci_dev * dev;
    int r = 0;
    unsigned long flags;

    if (find_dev_by_name(assigned_dev->name)) {
        printk(KERN_ERR "Deviced already assigned.\n");
        return -1;
    }

    assigned_dev = kmalloc(sizeof(struct pisces_host_pci_bdf), GFP_KERNEL);
    if (IS_ERR(assigned_dev)) {
        printk(KERN_ERR "Could not allocate space for assigned device\n");
        r = -ENOMEM;
        goto out_free;
    }

    strncpy(assigned_dev->name, bdf->name, 128);
    assigned_dev->domain = bdf->domain;
    assigned_dev->bus = bdf->bus;
    assigned_dev->devfn = PCI_DEVFN(bdf->dev, bdf->func);
    assigned_dev->intx_disabled = 1;
    spin_lock_init(&(assigned_dev->intx_lock));

    dev = pci_get_domain_bus_and_slot(
            assigned_dev->domain,
            assigned_dev->bus,
            assigned_dev->devfn);
    if (!dev) {
        printk(KERN_ERR "Assigned device not found\n");
        r = -EINVAL;
        goto out_free;
    }

    /* Don't allow bridges to be assigned */
    if (dev->hdr_type != PCI_HEADER_TYPE_NORMAL) {
        r = -EPERM;
        goto out_put;
    }

    /*
    r = probe_sysfs_permissions(dev);
    if (r)
        goto out_put;
    */

    if (pci_enable_device(dev)) {
        printk(KERN_ERR "Could not enable PCI device\n");
        r = -EBUSY;
        goto out_put;
    }

    r = pci_request_regions(dev, "pisces_assigned_device");
    if (r) {
        printk(KERN_INFO "Could not get access to device regions\n");
        goto out_disable;
    }

    pci_reset_function(dev);
    pci_save_state(dev);

    assigned_dev->dev = dev;
    spin_lock_irqsave(&assigned_device_list_lock, flags);
    list_add(&(assigned_dev->dev_node), &assigned_device_list);
    spin_unlock_irqrestore(&assigned_device_list_lock, flags);

    {
        int i = 0;
        for (i = 0; i < DEVICE_COUNT_RESOURCE; i++) {
            printk("Resource %d\n", i);
            printk("\tflags = 0x%lx\n", pci_resource_flags(dev, i));
            printk("\t name=%s, start=%lx, size=%d\n",
                    dev->resource[i].name,
                    (uintptr_t)pci_resource_start(dev, i),
                    (u32)pci_resource_len(dev, i));

        }

        printk("Rom BAR=%d\n", dev->rom_base_reg);
    }

    if (!iommu_present(&pci_bus_type)) {
        printk(KERN_ERR "iommu not found\n");
        r = -ENODEV;
        goto out_del_list;
    }
    assigned_dev->iommu_enabled = 1;

    return 0;

out_del_list:
    list_del(&assigned_device_list);
    pci_release_regions(dev);
out_disable:
    pci_disable_device(dev);
out_put:
    pci_dev_put(dev);
out_free:
    kfree(assigned_dev);
    return r;
}

int pisces_device_deassign(struct pisces_host_pci_bdf * bdf)
{
    return 0;
}

int enclave_pci_setup_lcall(
        struct pisces_enclave * enclave,
        struct pisces_cmd * cmd)
{
    struct pisces_pci_setup_cmd * pci_setup_cmd 
        = (struct pisces_pci_setup_cmd *) cmd;
    struct pisces_pci_setup_cmd * pci_setup_resp 
        = (struct pisces_pci_setup_cmd *) cmd;
    struct pisces_assigned_dev * assiged_dev;
    struct pci_dev * dev;

    assiged_dev = find_dev_by_name(pci_setup_cmd->name);
    if (assiged_dev == NULL) {
        printk(KERN_ERR "enclave_pci_setup_lcall deviced not found.\n");
        goto out;
    }

    dev = assiged_dev->dev;

    /* Cache first 6 BAR regs */
    {
        int i = 0;

        for (i = 0; i < 6; i++) {
            struct v3_host_pci_bar * bar = &(pci_setup_resp->bars[i]);
            unsigned long flags;

            bar->size = pci_resource_len(dev, i);
            bar->addr = pci_resource_start(dev, i);
            flags = pci_resource_flags(dev, i);

            if (flags & IORESOURCE_MEM) {
                if (flags & IORESOURCE_MEM_64) {
                    struct v3_host_pci_bar * hi_bar = &(pci_setup_resp->bars[i + 1]);
                    bar->type = PT_BAR_MEM64_LO;

                    hi_bar->type = PT_BAR_MEM64_HI;
                    hi_bar->size = bar->size;
                    hi_bar->addr = bar->addr;
                    hi_bar->cacheable = ((flags & IORESOURCE_CACHEABLE) != 0);
                    hi_bar->prefetchable = ((flags & IORESOURCE_PREFETCH) != 0);
                    i++;
                } else if (flags & IORESOURCE_DMA) {
                    bar->type = PT_BAR_MEM24;
                } else {
                    bar->type = PT_BAR_MEM32;
                }
                bar->cacheable = ((flags & IORESOURCE_CACHEABLE) != 0);
                bar->prefetchable = ((flags & IORESOURCE_PREFETCH) != 0);
            } else if (flags & IORESOURCE_IO) {
                bar->type = PT_BAR_IO;
            } else {
                bar->type = PT_BAR_NONE;
            }
        }
    }

    /* Cache expansion rom bar */
    {
        struct resource * rom_res = &(dev->resource[PCI_ROM_RESOURCE]);
        int rom_size = pci_resource_len(dev, PCI_ROM_RESOURCE);

        if (rom_size > 0) {
            unsigned long flags;

            pci_setup_resp->exp_rom.size = rom_size;
            pci_setup_resp->exp_rom.addr = pci_resource_start(dev, PCI_ROM_RESOURCE);
            flags = pci_resource_flags(dev, PCI_ROM_RESOURCE);

            pci_setup_resp->exp_rom.type = PT_EXP_ROM;

            pci_setup_resp->exp_rom.exp_rom_enabled = rom_res->flags & IORESOURCE_ROM_ENABLE;
        }
    }


    /* Cache entire configuration space */
    {
        int m = 0;

        // Copy the configuration space to the local cached version
        for (m = 0; m < PCI_HDR_SIZE; m += 4) {
            pci_read_config_dword(dev, m, (u32 *)&(pci_setup_resp->cfg_space[m]));
        }
    }

    /* IOMMU checked when assign device to Pisces */
    pci_setup_resp->iommu_present = 1;

    pci_setup_resp->resp.status = 0;
    pci_setup_resp->resp.data_len = pci_setup_cmd->cmd.data_len;
    return 0;

out:
    pci_setup_resp->resp.status = -1;
    pci_setup_resp->resp.data_len = 0;
    return 0;
}


int enclave_pci_request_deivce_lcall(
        struct pisces_enclave * enclave,
        struct pisces_cmd * cmd)
{
    /* iommu map guest */

    /* iommu attach device */

    return 0;
}

