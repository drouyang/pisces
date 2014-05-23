#include <linux/iommu.h>
#include <linux/pci.h>

#include "pisces_cmds.h"
#include "pisces_pci.h"
#include "pisces_sata.h"
#include "pgtables.h"


/* TODO: merge with pisces_pci_dev_init */
struct assigned_sata_dev *
pisces_sata_init(struct pisces_sata_dev * device) 
{
 
    return NULL;
}

int
pisces_sata_iommu_attach(struct assigned_sata_dev * assigned_dev)
{
    return -1;
}

int _pisces_sata_iommu_map(
        struct iommu_domain * iommu_domain,
        u64 start,
        u64 end,
        u64 gpa)
{
    return -1;
}


int
pisces_sata_iommu_map(struct assigned_sata_dev * assigned_dev)
{
  
    return -1;
}

void
pisces_enclave_add_disk(struct pisces_enclave *enclave)
{

	return;
}
