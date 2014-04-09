#include <linux/iommu.h>
#include <linux/pci.h>

#include "pisces_cmds.h"
#include "pisces_pci.h"
#include "pisces_sata.h"
#include "pgtables.h"

struct assigned_sata_dev * sata = NULL;

/* TODO: merge with pisces_pci_dev_init */
struct assigned_sata_dev *
pisces_sata_init(struct pisces_sata_dev * device) 
{
    struct pci_dev * dev;
    struct assigned_sata_dev * assigned_dev = NULL;

    printk("%s: %s at %d.%d:%d port %d\n", 
            __func__,
            device->name, device->bus, device->dev, device->func, device->port);

    assigned_dev = kmalloc(sizeof(struct pisces_assigned_dev), GFP_KERNEL);

    strncpy(assigned_dev->name, device->name, 128);
    assigned_dev->domain = 0;
    assigned_dev->bus = device->bus;
    assigned_dev->devfn = PCI_DEVFN(device->dev, device->func);
    assigned_dev->enclave = NULL;

    dev = pci_get_bus_and_slot(
            assigned_dev->bus,
            assigned_dev->devfn);
    if (!dev) {
        printk(KERN_ERR "Assigned device not found\n");
        goto out_free;
    }

    assigned_dev->dev = dev;

    /* iommu alloc */
    if (!iommu_present(&pci_bus_type)) {
        printk(KERN_ERR "IOMMU not found\n");
        goto out_free;
    }

    assigned_dev->iommu_domain = iommu_domain_alloc(&pci_bus_type);
    if (!assigned_dev->iommu_domain) {
        printk(KERN_ERR "iommu_domain_alloc error\n");
        goto out_free;
    }
    assigned_dev->iommu_enabled = 1;

    printk(KERN_INFO "Device %s initialized, iommu_domain allocated.\n", assigned_dev->name);

    /* global var */
    sata = assigned_dev;
    return assigned_dev;

out_free:
    kfree(assigned_dev);
    return NULL;
}

int
pisces_sata_iommu_attach(struct assigned_sata_dev * assigned_dev)
{
    int r = 0;

    r = iommu_attach_device(assigned_dev->iommu_domain, &assigned_dev->dev->dev);
    if (r) {
        printk(KERN_ERR "iommu_attach_device failed errno=%d\n", r);
        return r;
    }

    assigned_dev->dev->dev_flags |= PCI_DEV_FLAGS_ASSIGNED;
    printk(KERN_INFO "Device %s attached to iommu domain.\n", 
            assigned_dev->name);

    return r;
}

int _pisces_sata_iommu_map(
        struct iommu_domain * iommu_domain,
        u64 start,
        u64 end,
        u64 gpa)
{
    int r = 0;
    u64 flags = 0;
    u64 size = end - start;
    u32 page_size = 512 * 4096; // assume large 64bit pages (2MB)
    u64 hpa = start;

    flags = IOMMU_READ | IOMMU_WRITE;
    /* not sure if we need IOMMU_CACHE */
    //if (iommu_domain_has_cap(assigned_dev->iommu_domain, IOMMU_CAP_CACHE_COHERENCY)) {
    //    flags |= IOMMU_CACHE;
    //}

    do {
        if (size < page_size) {
            // less than a 2MB granularity, so we switch to small pages (4KB)
            page_size = 4096;
        }

        r = iommu_map(iommu_domain, gpa, hpa, page_size, flags);
        printk("Memory region: GPA=%p, HPA=%p, size=%p\n", (void *)gpa, (void *)hpa, (void *)size);
        if (r) {
            printk(KERN_ERR "%s: failed at gpa=%llx, hpa=%llx\n", 
                    __func__, gpa, hpa);
            return r;
        }

        hpa += page_size;
        gpa += page_size;
        size -= page_size;
    } while (size > 0);

    return r;
}


int
pisces_sata_iommu_map(struct assigned_sata_dev * assigned_dev)
{
    int r = 0;
    struct pisces_enclave * enclave = assigned_dev->enclave;

    r = _pisces_sata_iommu_map(
            assigned_dev->iommu_domain,
            enclave->bootmem_addr_pa,
            ((u64) PAGE_SIZE_2MB * 1024 * 4),
            0);
    if (r) {
        printk(KERN_ERR "%s: failed to map memory hpa %llx gpa %llx size %llx\n", 
                __func__, 
                (unsigned long long) enclave->bootmem_addr_pa,
                (unsigned long long) ((u64) PAGE_SIZE_2MB * 1024 * 4),
                (unsigned long long) 0
                );
        return r;
    }
#if 0
    {
        struct enclave_mem_block * mem_block = NULL;
        u64 size = 0;
        u64 gpa = 0;
        int i = 0;
        list_for_each_entry(mem_block, &(enclave->memdesc_list), node) {
            size = mem_block->pages * PAGE_SIZE_2MB;
            r = _pisces_sata_iommu_map(
                    assigned_dev->iommu_domain,
                    mem_block->base_addr,
                    size,
                    gpa);
            if (r) {
                printk(KERN_ERR "%s: failed to map mem_block %d\n", 
                        __func__, i);
                return r;
            }

            printk("%s: hpa %llx, gpa %llx, size %llx\n",
                    __func__,
                    (unsigned long long) mem_block->base_addr,
                    (unsigned long long) gpa,
                    (unsigned long long) size
                  );
            gpa += size;
            i++;
        }

        i = i-1;
        if (i != enclave->memdesc_num) {
            printk(KERN_WARNING "%s: %d out of %d mem_block mapped\n", 
                    __func__, i, enclave->memdesc_num);
        }
    }
#endif

    return r;
}

void
pisces_enclave_add_disk(struct pisces_enclave *enclave)
{
	struct pisces_sata_dev sata_dev;
	struct assigned_sata_dev * assigned_dev; 

	strcpy(sata_dev.name, "sata");
	sata_dev.bus = 0;
        sata_dev.dev = 0x1f;
        sata_dev.func = 2;
        sata_dev.port = 3;

	assigned_dev = pisces_sata_init(&sata_dev);

	if (assigned_dev == NULL) {
		printk(KERN_ERR "Error init pci device\n");
		return;
	}

	assigned_dev->enclave = enclave;

	pisces_sata_iommu_map(assigned_dev);
	pisces_sata_iommu_attach(assigned_dev);

	return;
}
