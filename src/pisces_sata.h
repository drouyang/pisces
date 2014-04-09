/*
 * SATA Assignment
 * (c) Jiannan Ouyang, 2014 (ouyang@cs.pitt.edu)
 */

#ifndef _PISCES_SATA_H_
#define _PISCES_SATA_H_

#include "pisces.h"
#include "enclave.h"
#include "pisces_cmds.h"

struct assigned_sata_dev {
  char name[128];
  u32 domain;
  u32 bus;
  u32 devfn;
  u32 port;

  struct pisces_enclave * enclave;

  struct pci_dev * dev;
  struct iommu_domain * iommu_domain;
  u8 iommu_enabled;
};

struct assigned_sata_dev *
pisces_sata_init(struct pisces_sata_dev * sata_dev);
int pisces_sata_iommu_attach(struct assigned_sata_dev * assigned_dev);
int pisces_sata_iommu_map(struct assigned_sata_dev * assigned_dev);
#endif

