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

#include "pisces_pci.h"

static LIST_HEAD(assigned_device_list);
static DEFINE_SPINLOCK(assigned_device_list_lock);

static struct pisces_assigned_dev * find_dev_by_name(char * name) {
    struct pisces_assigned_dev * dev = NULL;

    list_for_each_entry(dev, &assigned_device_list, dev_node) {
        if (strncmp(dev->name, name, 128) == 0) {
            return dev;
        }
    }

    return NULL;
}


int pisces_assign_device(struct pisces_host_pci_bdf * bdf)
{
    struct pisces_assigned_dev * assigned_dev = NULL;
    struct pci_dev * dev;
    int r = 0;
    unsigned long flags;

    spin_lock_irqsave(&assigned_device_list_lock, flags);
    if (find_dev_by_name(assigned_dev->name)) {
        spin_unlock_irqrestore(&assigned_device_list_lock, flags);
        printk(KERN_ERR "Deviced already assigned.\n");
        return -1;
    }
    spin_unlock_irqrestore(&assigned_device_list_lock, flags);

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

    /* Cache first 6 BAR regs ? */
    /* Cache expansion rom bar ? */
    /* Cache entire configuration space ? */

    if (!iommu_present(&pci_bus_type)) {
        printk(KERN_ERR "iommu not found\n");
        r = -ENODEV;
        goto out_del_list;
    }

    /* iommu map guest ? */
    /* iommu attach device? */

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
