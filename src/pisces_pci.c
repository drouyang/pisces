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

#include "ctrl_cmds.h"
#include "pisces_pci.h"
#include "pisces_lcall.h"
#include "enclave.h"
#include "ipi.h"

struct pisces_pci_dev {
    char name[128];
    u32  domain;
    u32  bus;
    u32  devfn;
    
    struct pisces_enclave * enclave;
    
    u8 in_use;
    u8 assigned;

    u8 iommu_enabled;
    struct iommu_domain * iommu_domain;
    
    u32 device_ipi_vector; /* for irq forwarding */
    
    spinlock_t intx_lock;
    u8         intx_disabled;
    
    struct pci_dev    * dev;


    struct list_head dev_node;
};



/** 
 * LCALL formats for PCI operations 
 */

struct pci_iommu_map_lcall {
    struct pisces_lcall      lcall;

    char name[128];
    u64  region_start;
    u64  region_end;
    u64  gpa;
} __attribute__((packed));

struct pci_iommu_unmap_lcall {
	struct pisces_lcall lcall;

	char name[128];
	u64 region_start;
	u64 region_end;
	u64 gpa;
} __attribute__((packed));


struct pci_attach_lcall {
    struct pisces_lcall      lcall;

    char name[128];
    u32  ipi_vector;
} __attribute__((packed));

struct pci_detach_lcall {
	struct pisces_lcall lcall;

	char name[128];
} __attribute__((packed));

struct pci_ack_irq_lcall {
    struct pisces_lcall      lcall;

    char name[128];
    u32  vector;
} __attribute__((packed));


typedef enum {
    HOST_PCI_CMD_DMA_DISABLE  = 1,
    HOST_PCI_CMD_DMA_ENABLE   = 2,
    HOST_PCI_CMD_MEM_ENABLE   = 3,
    HOST_PCI_CMD_INTX_DISABLE = 4,
    HOST_PCI_CMD_INTX_ENABLE  = 5,
} host_pci_cmd_t;



struct pci_cmd_lcall {
    struct pisces_lcall      lcall;

    char           name[128];
    host_pci_cmd_t cmd;
    u64            arg;
} __attribute__((packed));


/** 
 * End LCALL definitions 
 */



static struct pisces_pci_dev * 
find_dev_by_name(struct pisces_enclave * enclave,
		 char                  * name) 
{
    struct pisces_pci_state * pci_state = &(enclave->pci_state);
    struct pisces_pci_dev   * dev       = NULL;
    unsigned long flags;

    spin_lock_irqsave(&(pci_state->lock), flags);
    {
	if (list_empty(&(pci_state->dev_list))) {
	    spin_unlock_irqrestore(&(pci_state->lock), flags);
	    return NULL;
	}

	list_for_each_entry(dev, &(pci_state->dev_list), dev_node) {
	    if (strncmp(dev->name, name, 128) == 0) {
		spin_unlock_irqrestore(&(pci_state->lock), flags);
		return dev;
	    }
	}
    }
    spin_unlock_irqrestore(&(pci_state->lock), flags);

    return NULL;
}


static void
send_resp(struct pisces_xbuf_desc * xbuf_desc,
	  int                       status)
{
    struct pisces_lcall_resp resp;

    resp.status   = status;
    resp.data_len = 0;
    pisces_xbuf_complete(xbuf_desc, (u8 *)&resp,
			 sizeof(struct pisces_lcall_resp));
}

/*
 * Grab a PCI device and initialize it
 *  - ensures existence, non-bridge type, IOMMU enabled
 *  - enable device and add it to pci_device_list
 *
 * Prerequisite: the PCI device has to be offlined from Linux before
 * calling this function
 *
 * REF: linux: virt/kvm/assigned-dev.c
 */
int
pisces_pci_add_dev(struct pisces_enclave  * enclave,
		   struct pisces_pci_spec * spec)
{
    struct pisces_pci_state * pci_state = &(enclave->pci_state);
    struct pisces_pci_dev   * pci_dev   = NULL;
    struct pci_dev          * dev       = NULL;
    unsigned long flags;
    int ret = 0;

    if (find_dev_by_name(enclave, spec->name)) {
        printk(KERN_ERR "Device %s already assigned.\n", pci_dev->name);
        return -EINVAL;
    }

    pci_dev = kmalloc(sizeof(struct pisces_pci_dev), GFP_KERNEL);

    if (IS_ERR(pci_dev)) {
        printk(KERN_ERR "Could not allocate space for assigned device\n");
        ret = -ENOMEM;
        goto out_free;
    }

    strncpy(pci_dev->name, spec->name, 128);
    pci_dev->domain            = 0;
    pci_dev->bus               = spec->bus;
    pci_dev->devfn             = PCI_DEVFN(spec->dev, spec->func);
    pci_dev->device_ipi_vector = 0;
    pci_dev->intx_disabled     = 1;
    pci_dev->assigned          = 0;
    pci_dev->enclave           = enclave;
    spin_lock_init(&(pci_dev->intx_lock));

    /* equivilent pci_pci_get_domain_bus_and_slot(0, bus, devfn) */
    dev = pci_get_bus_and_slot(pci_dev->bus, pci_dev->devfn);

    if (!dev) {
        printk(KERN_ERR "Assigned device not found\n");
        ret = -EINVAL;
        goto out_free;
    }

    /* Don't allow bridges to be assigned */
    if (dev->hdr_type != PCI_HEADER_TYPE_NORMAL) {
        printk(KERN_ERR "Cannot assign a bridge\n");
        ret = -EPERM;
        goto out_put;
    }



    /*
    ret = probe_sysfs_permissions(dev);
    if (ret)
        goto out_put;
    */

    if (pci_enable_device(dev)) {
        printk(KERN_ERR "Could not enable PCI device\n");
        ret = -EBUSY;
        goto out_put;
    }

    ret = pci_request_regions(dev, "pisces_pci_device");

    if (ret) {
        printk(KERN_INFO "Could not get access to device regions\n");
        goto out_disable;
    }

    pci_reset_function(dev);

    /* Maybe save device state here? */

    pci_dev->dev = dev;


    spin_lock_irqsave(&(pci_state->lock), flags);
    {
	list_add(&(pci_dev->dev_node), &(pci_state->dev_list));
    }
    spin_unlock_irqrestore(&(pci_state->lock), flags);


    /* iommu init */
    if (!iommu_present(&pci_bus_type)) {
        printk(KERN_ERR "IOMMU not found\n");
        ret = -ENODEV;
        goto out_del_list;
    }

    pci_dev->iommu_domain = iommu_domain_alloc(&pci_bus_type);

    if (!pci_dev->iommu_domain) {
        printk(KERN_ERR "iommu_domain_alloc error\n");
        ret = -ENOMEM;
        goto out_del_list;
    }

    pci_dev->iommu_enabled = 1;

    printk(KERN_INFO "Device %s initialized, iommu_domain allocated.\n", pci_dev->name);

    return 0;

out_del_list:
    list_del(&(pci_dev->dev_node));
    pci_release_regions(dev);
out_disable:
    pci_disable_device(dev);
out_put:
    pci_dev_put(dev);
out_free:
    kfree(pci_dev);

    return ret;
}

int
pisces_pci_remove_dev(struct pisces_enclave  * enclave,
		      struct pisces_pci_spec * spec)
{
    return -1;
}


static irqreturn_t 
_host_pci_intx_irq_handler(int    irq, 
			   void * priv_data)
{
    struct pisces_pci_dev * pci_dev = priv_data;

    //    printk("Passthrough PCI INTX handler (irq %d)\n", irq);

    spin_lock(&(pci_dev->intx_lock));
    {
	disable_irq_nosync(irq);
    }
    spin_unlock(&(pci_dev->intx_lock));



    /* For now the second parameter (cpu_id) is not used
     * in pisces_send_ipi, and IPIs are always send to
     * to enclave->boot_cpu 
     */
    if (pci_dev->enclave == NULL) {
        printk("Error: device %s has NULL enclave field\n", pci_dev->name);
        return IRQ_NONE;
    }

    if (pci_dev->device_ipi_vector == 0) {
        printk("Error: device %s ipi vector not initialized\n", pci_dev->name);
        return IRQ_NONE;
    }

    pisces_send_ipi(pci_dev->enclave, 0, pci_dev->device_ipi_vector);


    return IRQ_HANDLED;
}




int
pisces_pci_iommu_map(struct pisces_enclave      * enclave,
		     struct pisces_xbuf_desc    * xbuf_desc,
		     struct pci_iommu_map_lcall * lcall)
{
    struct pisces_pci_dev * pci_dev = NULL;
    int ret  = 0;

    pci_dev = find_dev_by_name(enclave, lcall->name);

    if (pci_dev == NULL) {
        printk(KERN_ERR "iommu_map device %s not found.\n", lcall->name);
	send_resp(xbuf_desc, -1);
	return 0;
    }

    {
	u64 size      = lcall->region_end - lcall->region_start;
	u32 page_size = 512 * 4096; // assume large 64bit pages (2MB)
	u64 hpa       = lcall->region_start;
	u64 gpa       = lcall->gpa;
	u64 flags     = IOMMU_READ | IOMMU_WRITE;

	/* not sure if we need IOMMU_CACHE */
	//if (iommu_domain_has_cap(pci_dev->iommu_domain, IOMMU_CAP_CACHE_COHERENCY)) {
	//    flags |= IOMMU_CACHE;
	//}

	printk("Memory region: GPA=%p, HPA=%p, size=%p\n", (void *)gpa, (void *)hpa, (void *)size);

	do {
	    if (size < page_size) {
		// less than a 2MB granularity, so we switch to small pages (4KB)
		page_size = 4096;
	    }

	    ret = iommu_map(pci_dev->iommu_domain, gpa, hpa, page_size, flags);

	    if (ret) {
		printk(KERN_ERR "iommu_map failed for device %s at gpa=%llx, hpa=%llx\n", pci_dev->name, gpa, hpa);
		break;
	    }

	    hpa  += page_size;
	    gpa  += page_size;
	    size -= page_size;

	} while (size > 0);

    }
    
    send_resp(xbuf_desc, ret);
    return 0;
}


int
pisces_pci_iommu_unmap(struct pisces_enclave      * enclave,
		       struct pisces_xbuf_desc    * xbuf_desc,
		       struct pci_iommu_unmap_lcall * lcall)
{
    struct pisces_pci_dev * pci_dev = NULL;
    int ret  = 0;

    pci_dev = find_dev_by_name(enclave, lcall->name);

    if (pci_dev == NULL) {
        printk(KERN_ERR "iommu_map device %s not found.\n", lcall->name);
	send_resp(xbuf_desc, -1);
	return 0;
    }

    {
	u64 size       = lcall->region_end - lcall->region_start;
	u32 page_size  = 512 * 4096; // assume large 64bit pages (2MB)
	u64 gpa        = lcall->gpa;
	u64 unmap_size = 0;

	/* not sure if we need IOMMU_CACHE */
	//if (iommu_domain_has_cap(pci_dev->iommu_domain, IOMMU_CAP_CACHE_COHERENCY)) {
	//    flags |= IOMMU_CACHE;
	//}

	printk("Memory region: GPA=%p, size=%p\n", (void *)gpa, (void *)size);

	do {
	    if (size < page_size) {
		// less than a 2MB granularity, so we switch to small pages (4KB)
		page_size = 4096;
	    }

	    unmap_size = iommu_unmap(pci_dev->iommu_domain, gpa, page_size);

	    /* We should NOT have any holes in our mappings
	     *    We might in the future, in which case this would need to change
	     */
	    if (unmap_size != page_size) {
		printk(KERN_ERR "iommu_unmap failed for device %s at gpa=%llx\n", pci_dev->name, gpa);
		ret = -1;
		break;
	    }

	    gpa  += page_size;
	    size -= page_size;

	} while (size > 0);

    }
    
    send_resp(xbuf_desc, ret);

    return 0;
}

int
pisces_pci_attach(struct pisces_enclave   * enclave,
		  struct pisces_xbuf_desc * xbuf_desc,
		  struct pci_attach_lcall * lcall)
{
    struct pisces_pci_dev * pci_dev =  find_dev_by_name(enclave, lcall->name);
    int ret = 0;

     if (pci_dev == NULL) {
        printk(KERN_ERR "iommu_map device %s not found.\n", lcall->name);
	send_resp(xbuf_desc, -1);
	return 0;
    }
    
    if (pci_dev->assigned) {
        printk(KERN_ERR "device %s already assigned.\n", pci_dev->name);
        send_resp(xbuf_desc, -1);
	return 0;
    }

    ret = iommu_attach_device(pci_dev->iommu_domain, &pci_dev->dev->dev);

    if (ret) {
        printk(KERN_ERR "iommu_attach_device failed errno=%d\n", ret);
        return ret;
    }

    pci_dev->dev->dev_flags   |= PCI_DEV_FLAGS_ASSIGNED;
    pci_dev->assigned          = 1;
    pci_dev->device_ipi_vector = lcall->ipi_vector;

    printk(KERN_INFO "Device %s attached to iommu domain.\n", pci_dev->name);

    /* Request the IRQ at attach, in case the driver forgets to enable it (?) */
    {
	if (request_threaded_irq(pci_dev->dev->irq, NULL, _host_pci_intx_irq_handler, 
				 IRQF_ONESHOT, "V3Vee_Host_PCI_INTx", (void *)pci_dev)) {
	    
	    printk("ERROR assigning IRQ to assigned PCI device (%s)\n", 
		   pci_dev->name);
	}
	
	pci_dev->intx_disabled = 0;
    }
    
    send_resp(xbuf_desc, 0);
    return 0;
}


int pisces_pci_detach(struct pisces_enclave   * enclave,
		      struct pisces_xbuf_desc * xbuf_desc,
		      struct pci_detach_lcall * lcall)
{
    struct pisces_pci_dev * pci_dev =  find_dev_by_name(enclave, lcall->name);

    if (pci_dev == NULL) {
        printk(KERN_ERR "iommu_map device %s not found.\n", lcall->name);
	send_resp(xbuf_desc, -1);
	return 0;
    }
    
    if (!pci_dev->assigned) {
        printk(KERN_ERR "device %s has not been assigned.\n", pci_dev->name);
        send_resp(xbuf_desc, -1);
	return 0;
    }

    iommu_detach_device(pci_dev->iommu_domain, &pci_dev->dev->dev);

    
    pci_dev->dev->dev_flags   &= ~PCI_DEV_FLAGS_ASSIGNED;
    pci_dev->assigned          =  0;
    pci_dev->device_ipi_vector =  0;

    printk(KERN_INFO "Device %s detached from iommu domain.\n", pci_dev->name);
   
    send_resp(xbuf_desc, 0);
    return 0;
}


int 
pisces_pci_ack_irq(struct pisces_enclave    * enclave,
		   struct pisces_xbuf_desc  * xbuf_desc,
		   struct pci_ack_irq_lcall * lcall)
{
    struct pisces_pci_dev    * pci_dev = NULL;

    char * name   = lcall->name;

    unsigned long flags;
 
    pci_dev = find_dev_by_name(enclave, name);

    if (pci_dev == NULL) {
        printk(KERN_ERR "pci_ack_irq device %s not found.\n", name);
	send_resp(xbuf_desc, -1);
	return 0;
    }

    //    printk("Acking IRQ vector %d\n", vector);

    spin_lock_irqsave(&(pci_dev->intx_lock), flags);
    {
	//printk("Enabling IRQ %d\n", dev->irq);
	enable_irq(pci_dev->dev->irq);
    }
    spin_unlock_irqrestore(&(pci_dev->intx_lock), flags);

    send_resp(xbuf_desc, 0);
    return 0;
}

int 
pisces_pci_cmd(struct pisces_enclave   * enclave,
	       struct pisces_xbuf_desc * xbuf_desc,
	       struct pci_cmd_lcall    * lcall)
{
    struct pisces_pci_dev    * pci_dev = NULL;

    host_pci_cmd_t  cmd  = lcall->cmd;
    char          * name = lcall->name;

    pci_dev = find_dev_by_name(enclave, name);

    if (pci_dev == NULL) {
        printk(KERN_ERR "pci_cmd device %s not found.\n", name);
	send_resp(xbuf_desc, -1);
	return 0;
    }


    switch (cmd) {
          case HOST_PCI_CMD_INTX_DISABLE:
            printk("Passthrough PCI device disabling INTx IRQ\n");

	    if (pci_dev->intx_disabled == 1) {
		printk("INTx already disabled for Host PCI Device (%s)\n", name);
		break;
	    }
	    
	    disable_irq(pci_dev->dev->irq);
	    free_irq(pci_dev->dev->irq, (void *)pci_dev);
	    pci_dev->intx_disabled = 1;

            break;

        case HOST_PCI_CMD_INTX_ENABLE:
            printk("Passthrough PCI device enabling INTx IRQ\n");
	    
	    if (pci_dev->intx_disabled == 0) {
		printk("INTx already enabled for Host PCI Device (%s)\n", name);
		break;
	    }

	    if (request_threaded_irq(pci_dev->dev->irq, NULL,  _host_pci_intx_irq_handler, 
				     IRQF_ONESHOT,  "V3Vee_Host_PCI_INTx", (void *)pci_dev)) {
		
		printk("ERROR assigning IRQ to assigned PCI device (%s)\n", 
		       pci_dev->name);
	    }
	    
	    pci_dev->intx_disabled = 0;

            break;
        default:
            printk("Error: Invalid passthrough PCI command: %d\n", cmd);
	    send_resp(xbuf_desc, -1);
	    return 0;
    }


    send_resp(xbuf_desc, 0);

    return 0;
}




int
pisces_pci_init(struct pisces_enclave * enclave) 
{
    struct pisces_pci_state * pci_state = &(enclave->pci_state);

    spin_lock_init(&(pci_state->lock));
    INIT_LIST_HEAD(&(pci_state->dev_list));
    pci_state->dev_num = 0;
    
    return 0;
}
