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

#include "pisces_cmds.h"
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
 * Grab a PCI device and initialize it
 *  - ensures existence, non-bridge type, IOMMU enabled
 *  - enable device and add it to assigned_device_list
 *
 * Prerequisite: the PCI device has to be offlined from Linux before
 * calling this function
 *
 * REF: linux: virt/kvm/assigned-dev.c
 */
int pisces_pci_dev_get(struct pisces_pci_dev *device)
{
    struct pisces_assigned_dev * assigned_dev = NULL;
    struct pci_dev * dev;
    int r = 0;
    unsigned long flags;

    if (find_dev_by_name(device->name)) {
        printk(KERN_ERR "Deviced %s already assigned.\n", assigned_dev->name);
        return -1;
    }

    assigned_dev = kmalloc(sizeof(struct pisces_assigned_dev), GFP_KERNEL);

    if (IS_ERR(assigned_dev)) {
        printk(KERN_ERR "Could not allocate space for assigned device\n");
        r = -ENOMEM;
        goto out_free;
    }

    strncpy(assigned_dev->name, device->name, 128);
    assigned_dev->domain = device->domain;
    assigned_dev->bus = device->bus;
    assigned_dev->devfn = PCI_DEVFN(device->dev, device->func);
    assigned_dev->intx_disabled = 1;
    spin_lock_init(&(assigned_dev->intx_lock));

    /* equivilent pci_pci_get_domain_bus_and_slot(0, bus, devfn) */
    dev = pci_get_bus_and_slot(
            assigned_dev->bus,
            assigned_dev->devfn);

    if (!dev) {
        printk(KERN_ERR "Assigned device not found\n");
        r = -EINVAL;
        goto out_free;
    }

    /* Don't allow bridges to be assigned */
    if (dev->hdr_type != PCI_HEADER_TYPE_NORMAL) {
        printk(KERN_ERR "Cannot assign a bridge\n");
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

    /* Maybe save device state here? */

    assigned_dev->dev = dev;
    spin_lock_irqsave(&assigned_device_list_lock, flags);
    list_add(&(assigned_dev->dev_node), &assigned_device_list);
    spin_unlock_irqrestore(&assigned_device_list_lock, flags);


    if (!iommu_present(&pci_bus_type)) {
        printk(KERN_ERR "IOMMU not found\n");
        r = -ENODEV;
        goto out_del_list;
    }
    assigned_dev->iommu_enabled = 1;

    printk(KERN_INFO "Device %s assigned.\n", assigned_dev->name);

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

#if 0
/*
 * Reserve a PCI device
 *  - ensures existence, non-bridge type, IOMMU enabled
 *  - enable device and add it to assigned_device_list
 */
int pisces_device_assign(struct pisces_assigned_dev * bdf)
{
    struct pisces_assigned_dev * assigned_dev = NULL;
    struct pci_dev * dev;
    int r = 0;
    unsigned long flags;

    if (find_dev_by_name(assigned_dev->name)) {
        printk(KERN_ERR "Deviced %s already assigned.\n", assigned_dev->name);
        return -1;
    }

    assigned_dev = kmalloc(sizeof(struct pisces_assigned_dev), GFP_KERNEL);

    if (IS_ERR(assigned_dev)) {
        printk(KERN_ERR "Could not allocate space for assigned device\n");
        r = -ENOMEM;
        goto out_free;
    }

    strncpy(assigned_dev->name, bdf->name, 128);
    assigned_dev->domain = bdf->domain;
    assigned_dev->bus = bdf->bus;
    //assigned_dev->devfn = PCI_DEVFN(bdf->dev, bdf->func);
    assigned_dev->intx_disabled = 1;
    spin_lock_init(&(assigned_dev->intx_lock));

    dev = pci_get_bus_and_slot(

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


    if (!iommu_present(&pci_bus_type)) {
        printk(KERN_ERR "IOMMU not found\n");
        r = -ENODEV;
        goto out_del_list;
    }
    assigned_dev->iommu_enabled = 1;

    printk(KERN_INFO "Device %s assigned.\n", assigned_dev->name);

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
#endif

