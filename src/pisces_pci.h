/*
 * Device Assignment
 * (c) Jiannan Ouyang, 2013 (ouyang@cs.pitt.edu)
 */

#ifndef _PISCES_PCI_H_
#define _PISCES_PCI_H_

#include "pisces.h"
#include "enclave.h"
#include "pisces_cmds.h"

struct pisces_assigned_dev {
  char name[128];
  u32 domain;
  u32 bus;
  u32 devfn;

  struct list_head dev_node;

  u8 in_use;
  u8 iommu_enabled;
  struct iommu_domain * iommu_domain;

  enum {INTX_IRQ, MSI_IRQ, MSIX_IRQ} irq_type;
  uint32_t num_vecs;
  spinlock_t intx_lock;
  u8 intx_disabled;
  u32 num_msix_vecs;
  struct msix_entry * msix_entries;

  struct pci_dev * dev;
};


/* Palacios Specific Definations */

typedef enum {
  HOST_PCI_CMD_DMA_DISABLE = 1,
  HOST_PCI_CMD_DMA_ENABLE = 2,
  HOST_PCI_CMD_INTX_DISABLE = 3,
  HOST_PCI_CMD_INTX_ENABLE = 4,
  HOST_PCI_CMD_MSI_DISABLE = 5,
  HOST_PCI_CMD_MSI_ENABLE = 6,
  HOST_PCI_CMD_MSIX_DISABLE = 7,
  HOST_PCI_CMD_MSIX_ENABLE = 8
} host_pci_cmd_t;





int pisces_device_assign(struct pisces_host_pci * bdf);

#endif
