/*
 * Device Assignment
 * (c) Jiannan Ouyang, 2013 (ouyang@cs.pitt.edu)
 */

#ifndef _PISCES_PCI_H_
#define _PISCES_PCI_H_

#include "pisces.h"

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

int pisces_assign_device(struct pisces_host_pci_bdf * bdf);

#endif
