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

#include "pisces_cmds.h"
#include "pisces_pci.h"
#include "ipi.h"

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
    if (list_empty(&assigned_device_list)) {
        spin_unlock_irqrestore(&assigned_device_list_lock, flags);
        return NULL;
    }

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
struct pisces_assigned_dev *
pisces_pci_dev_init(struct pisces_pci_dev *device)
{
    struct pisces_assigned_dev * assigned_dev = NULL;
    struct pci_dev * dev;
    int r = 0;
    unsigned long flags;

    if (find_dev_by_name(device->name)) {
        printk(KERN_ERR "Device %s already assigned.\n", assigned_dev->name);
        return NULL;
    }

    assigned_dev = kmalloc(sizeof(struct pisces_assigned_dev), GFP_KERNEL);

    if (IS_ERR(assigned_dev)) {
        printk(KERN_ERR "Could not allocate space for assigned device\n");
        r = -ENOMEM;
        goto out_free;
    }

    strncpy(assigned_dev->name, device->name, 128);
    assigned_dev->domain = 0;
    assigned_dev->bus = device->bus;
    assigned_dev->devfn = PCI_DEVFN(device->dev, device->func);
    assigned_dev->device_ipi_vector = 0;
    assigned_dev->intx_disabled = 1;
    assigned_dev->assigned = 0;
    assigned_dev->enclave = NULL;
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


    /* iommu init */
    if (!iommu_present(&pci_bus_type)) {
        printk(KERN_ERR "IOMMU not found\n");
        r = -ENODEV;
        goto out_del_list;
    }
    assigned_dev->iommu_domain = iommu_domain_alloc(&pci_bus_type);
    if (!assigned_dev->iommu_domain) {
        printk(KERN_ERR "iommu_domain_alloc error\n");
        r = -ENOMEM;
        goto out_del_list;
    }
    assigned_dev->iommu_enabled = 1;

    printk(KERN_INFO "Device %s initialized, iommu_domain allocated.\n", assigned_dev->name);

    return assigned_dev;

out_del_list:
    list_del(&assigned_device_list);
    pci_release_regions(dev);
out_disable:
    pci_disable_device(dev);
out_put:
    pci_dev_put(dev);
out_free:
    kfree(assigned_dev);
    return NULL;
}

static int  _pisces_pci_ack_irq(
        struct pisces_assigned_dev * assigned_dev, 
        u32 vector)
{
    struct pci_dev * dev = assigned_dev->dev;
    unsigned long flags;

    //    printk("Acking IRQ vector %d\n", vector);

    spin_lock_irqsave(&(assigned_dev->intx_lock), flags);
    //printk("Enabling IRQ %d\n", dev->irq);
    enable_irq(dev->irq);
    assigned_dev->intx_disabled = 0;
    spin_unlock_irqrestore(&(assigned_dev->intx_lock), flags);

    return 0;
}


/* forward irq through IPI */
int _pisces_pci_raise_irq(struct pisces_assigned_dev * assigned_dev)
{

    /* For now the second parameter (cpu_id) is not used
     * in pisces_send_ipi, and IPIs are always send to
     * to enclave->boot_cpu 
     */
    if (assigned_dev->enclave == NULL) {
        printk("Error: device %s has NULL enclave field\n", assigned_dev->name);
        return -1;
    }

    if (assigned_dev->device_ipi_vector == 0) {
        printk("Error: device %s ipi vector not initialized\n", assigned_dev->name);
        return -1;
    }

    return pisces_send_ipi(
            assigned_dev->enclave, 
            0, 
            assigned_dev->device_ipi_vector);
}



static irqreturn_t _host_pci_intx_irq_handler(int irq, void * priv_data) {
    struct pisces_assigned_dev * assigned_dev = priv_data;

    //    printk("Passthrough PCI INTX handler (irq %d)\n", irq);

    spin_lock(&(assigned_dev->intx_lock));
    disable_irq_nosync(irq);
    assigned_dev->intx_disabled = 1;
    spin_unlock(&(assigned_dev->intx_lock));

    _pisces_pci_raise_irq(assigned_dev);

    return IRQ_HANDLED;
}

static int _pisces_pci_iommu_map_region(struct pisces_assigned_dev *assigned_dev, u64 start, u64 end, u64 gpa)
{
    u64 flags = 0;
    int r = 0;
    u64 size = end - start;
    u32 page_size = 512 * 4096; // assume large 64bit pages (2MB)
    u64 hpa = start;

    flags = IOMMU_READ | IOMMU_WRITE;
    /* not sure if we need IOMMU_CACHE */
    //if (iommu_domain_has_cap(assigned_dev->iommu_domain, IOMMU_CAP_CACHE_COHERENCY)) {
    //    flags |= IOMMU_CACHE;
    //}

    printk("Memory region: GPA=%p, HPA=%p, size=%p\n", (void *)gpa, (void *)hpa, (void *)size);

    do {
        if (size < page_size) {
            // less than a 2MB granularity, so we switch to small pages (4KB)
            page_size = 4096;
        }

        r = iommu_map(assigned_dev->iommu_domain, gpa, hpa, page_size, flags);
        if (r) {
            printk(KERN_ERR "iommu_map failed for device %s at gpa=%llx, hpa=%llx\n", assigned_dev->name, gpa, hpa);
            return r;
        }

        hpa += page_size;
        gpa += page_size;
        size -= page_size;
    } while (size > 0);



    return r;
}

static int _pisces_pci_iommu_attach_device(struct pisces_assigned_dev *assigned_dev)
{
    int r = 0;
    struct pci_dev * dev = assigned_dev->dev;

    if (assigned_dev->assigned) {
        printk(KERN_ERR "device %s already assigned.\n", assigned_dev->name);
        return -1;
    }

    r = iommu_attach_device(assigned_dev->iommu_domain, &assigned_dev->dev->dev);
    if (r) {
        printk(KERN_ERR "iommu_attach_device failed errno=%d\n", r);
        return r;
    }

    assigned_dev->dev->dev_flags |= PCI_DEV_FLAGS_ASSIGNED;
    assigned_dev->assigned = 1;
    printk(KERN_INFO "Device %s attached to iommu domain.\n", assigned_dev->name);

    if (request_threaded_irq(dev->irq, NULL,  _host_pci_intx_irq_handler, 
		    IRQF_ONESHOT,  "V3Vee_Host_PCI_INTx", (void *)assigned_dev)) {
	
	printk("ERROR assigning IRQ to assigned PCI device (%s)\n", 
	       assigned_dev->name);
    }
    
    return r;
}




#if 0
static irqreturn_t _host_pci_msi_irq_handler(int irq, void * priv_data) {
    struct pisces_assigned_dev * assigned_dev = priv_data;

    printk("Passthrough PCI MSI handler (irq %d)\n", irq);

    _pisces_pci_raise_irq(assigned_dev);

    return IRQ_HANDLED;
}

static irqreturn_t _host_pci_msix_irq_handler(int irq, void * priv_data) {
    int i;
    struct pisces_assigned_dev * assigned_dev = priv_data;

    printk("Passthrough PCI MSIX handler (irq %d)\n", irq);

    _pisces_pci_raise_irq(assigned_dev);

    for (i = 0; i < assigned_dev->num_msix_vecs; i++) {
        if (irq == assigned_dev->msix_entries[i].vector) {
            /* DO NOT SUPPORT multiple vector over IPI */
            //_pisces_pci_raise_irq(assigned_dev, i);
        } else {
            printk("Error matching MSIX vector for IRQ %d\n", irq);
        }
    }

    return IRQ_HANDLED;
}
#endif

static int _pisces_pci_cmd(
        struct pisces_assigned_dev * assigned_dev,
        host_pci_cmd_t cmd,
        u64 arg)
{
    struct pci_dev * dev = assigned_dev->dev;
    int status = 0;

    switch (cmd) {
        case HOST_PCI_CMD_DMA_DISABLE:
            printk("Passthrough PCI device disabling BMDMA\n");
            pci_clear_master(dev);
            break;

        case HOST_PCI_CMD_DMA_ENABLE:
            printk("Passthrough PCI device enabling BMDMA\n");
            pci_set_master(dev);
            break;

        case HOST_PCI_CMD_MEM_ENABLE: {
	    u16 hw_cmd = 0;
            printk("Passthrough PCI device enabling MEM resources\n");
	    
	    //            status = pci_enable_device_mem(dev);

            pci_read_config_word(dev, PCI_COMMAND, &hw_cmd);
            hw_cmd |= 0x2;
            pci_write_config_word(dev, PCI_COMMAND, hw_cmd);

            if (status)
                return status;

            break;
	}
        case HOST_PCI_CMD_INTX_DISABLE:
            printk("Passthrough PCI device disabling INTx IRQ\n");

            disable_irq(dev->irq);
            free_irq(dev->irq, (void *)assigned_dev);

            break;

        case HOST_PCI_CMD_INTX_ENABLE:
            printk("Passthrough PCI device enabling INTx IRQ\n");

            if (request_threaded_irq(dev->irq, NULL,  _host_pci_intx_irq_handler, 
			    IRQF_ONESHOT,  "V3Vee_Host_PCI_INTx", (void *)assigned_dev)) {

                printk("ERROR assigning IRQ to assigned PCI device (%s)\n", 
                        assigned_dev->name);
            }

            break;

        case HOST_PCI_CMD_MSI_DISABLE:
            printk("Passthrough PCI device disabling MSI\n");

            //disable_irq(dev->irq);
            //free_irq(dev->irq, (void *)assigned_dev);

            //pci_disable_msi(dev);

            break;

        case HOST_PCI_CMD_MSI_ENABLE:
            printk("Passthrough PCI device enabling MSI\n");

#if 0
            if (!dev->msi_enabled) {
                if (pci_enable_msi(dev) != 0) {
                    printk(KERN_ERR "Error enabling MSI for assigned device %s\n", assigned_dev->name);
                    return -1;
                }
            }

            printk("  MSI enabled\n");

            if (request_irq(dev->irq, 
                        _host_pci_msi_irq_handler, 
                        0, 
                        "V3Vee_host_PCI_MSI", 
                        (void *)assigned_dev)) {
                printk("Error requesting MSI IRQ %d for assigned device %s\n", 
                        dev->irq, assigned_dev->name);
                pci_disable_msi(dev);
                return -1;
            }

            printk(KERN_ERR "IRQ requested\n");
#endif

            break;

        case HOST_PCI_CMD_MSIX_ENABLE:
            printk("PCI_CMD_MSIX_ENABLE not supported\n");
            break;

        case HOST_PCI_CMD_MSIX_DISABLE: 
            printk("PCI_CMD_MSIX_DISABLE not supported\n");

#if 0
        case HOST_PCI_CMD_MSIX_ENABLE:
            printk("Passthrough PCI device enabling MSIX\n");

            {
                int i = 0;

                assigned_dev->num_msix_vecs = arg;
                assigned_dev->msix_entries = kcalloc(
                        assigned_dev->num_msix_vecs, 
                        sizeof(struct msix_entry), GFP_KERNEL);

                for (i = 0; i < assigned_dev->num_msix_vecs; i++) {
                    assigned_dev->msix_entries[i].entry = i;
                }

                pci_enable_msix(dev, 
                        assigned_dev->msix_entries, 
                        assigned_dev->num_msix_vecs);

                for (i = 0; i < assigned_dev->num_msix_vecs; i++) {
                    if (request_irq(assigned_dev->msix_entries[i].vector, 
                                _host_pci_msix_irq_handler, 
                                0, 
                                "V3VEE_host_PCI_MSIX", 
                                (void *)assigned_dev)) {
                        printk("Error requesting MSIX IRQ %d"
                                "for passthrough device %s\n", 
                                assigned_dev->msix_entries[i].vector,
                                assigned_dev->name);
                    }
                }

                break;
            }

        case HOST_PCI_CMD_MSIX_DISABLE: 
            printk("Passthrough PCI device Disabling MSIX\n");

            {
                int i = 0;

                for (i = 0; i < assigned_dev->num_msix_vecs; i++) {
                    disable_irq(assigned_dev->msix_entries[i].vector);
                }

                for (i = 0; i < assigned_dev->num_msix_vecs; i++) {
                    free_irq(assigned_dev->msix_entries[i].vector, (void *)assigned_dev);
                }

                assigned_dev->num_msix_vecs = 0;
                kfree(assigned_dev->msix_entries);

                pci_disable_msix(dev);

                break;
            }
#endif

        default:
            printk("Error: unhandled passthrough PCI command: %d\n", cmd);
            return -1;
    }

    return 0;
}

int pisces_pci_iommu_map(
    struct pisces_enclave * enclave,
    struct pisces_xbuf_desc * xbuf_desc,
    struct pisces_pci_iommu_map_lcall * lcall)
{
    int r = 0;
    struct pisces_lcall_resp iommu_map_resp;
    struct pisces_assigned_dev *assigned_dev = NULL;
    char *name = lcall->name;
    u64 start = lcall->region_start;
    u64 end = lcall->region_end;
    u64 gpa = lcall->gpa;
    int last = lcall->last;

    assigned_dev = find_dev_by_name(name);
    if (assigned_dev == NULL) {
        printk(KERN_ERR "iommu_map device %s not found.\n", name);
        r = -1;
        goto out;
    }

    if (!last) {
        r = _pisces_pci_iommu_map_region(assigned_dev, start, end, gpa);
    } else {
        r = _pisces_pci_iommu_attach_device(assigned_dev);
    }

out:
    iommu_map_resp.status = r;
    iommu_map_resp.data_len = 0;
    pisces_xbuf_complete(xbuf_desc, (u8 *)&iommu_map_resp, 
            sizeof(struct pisces_lcall_resp));
    return 0;
}

int pisces_pci_ack_irq(
    struct pisces_enclave * enclave,
    struct pisces_xbuf_desc * xbuf_desc,
    struct pisces_pci_ack_irq_lcall * lcall)
{
    int r = 0;
    struct pisces_lcall_resp resp;
    struct pisces_assigned_dev *assigned_dev = NULL;
    char *name = lcall->name;
    u32 vector = lcall->vector;

    assigned_dev = find_dev_by_name(name);
    if (assigned_dev == NULL) {
        printk(KERN_ERR "pci_ack_irq device %s not found.\n", name);
        r = -1;
        goto out;
    }

    r = _pisces_pci_ack_irq(assigned_dev, vector);

out:
    resp.status = r;
    resp.data_len = 0;
    pisces_xbuf_complete(xbuf_desc, (u8 *)&resp,
            sizeof(struct pisces_lcall_resp));
    return 0;
}

int pisces_pci_cmd(
    struct pisces_enclave * enclave,
    struct pisces_xbuf_desc * xbuf_desc,
    struct pisces_pci_cmd_lcall * lcall)
{
    int r = 0;
    struct pisces_lcall_resp resp;
    struct pisces_assigned_dev *assigned_dev = NULL;
    char *name = lcall->name;
    host_pci_cmd_t pci_cmd = lcall->cmd;
    u64 arg = lcall->arg;

    assigned_dev = find_dev_by_name(name);
    if (assigned_dev == NULL) {
        printk(KERN_ERR "pci_cmd device %s not found.\n", name);
        r = -1;
        goto out;
    }

    r = _pisces_pci_cmd(assigned_dev, pci_cmd, arg);

out:
    resp.status = r;
    resp.data_len = 0;
    pisces_xbuf_complete(xbuf_desc, (u8 *)&resp,
            sizeof(struct pisces_lcall_resp));
    return 0;
}
