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

  struct pisces_enclave * enclave;
  u32 device_ipi_vector; /* for irq forwarding */

  struct list_head dev_node;

  u8 in_use;
  u8 iommu_enabled;
  u8 assigned;
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
  HOST_PCI_CMD_MEM_ENABLE = 3,
  HOST_PCI_CMD_INTX_DISABLE = 4,
  HOST_PCI_CMD_INTX_ENABLE = 5,
  HOST_PCI_CMD_MSI_DISABLE = 6,
  HOST_PCI_CMD_MSI_ENABLE = 7,
  HOST_PCI_CMD_MSIX_DISABLE = 8,
  HOST_PCI_CMD_MSIX_ENABLE = 9
} host_pci_cmd_t;

struct pisces_pci_iommu_map_lcall {
    union {
        struct pisces_lcall lcall;
        struct pisces_lcall_resp lcall_resp;
    } __attribute__((packed));
    char name[128];
    u64 region_start;
    u64 region_end;
    u64 gpa;
    u32 last;
} __attribute__((packed));

struct pisces_pci_ack_irq_lcall {
    union {
        struct pisces_lcall lcall;
        struct pisces_lcall_resp lcall_resp;
    } __attribute__((packed));
    char name[128];
    u32 vector;
} __attribute__((packed));

struct pisces_pci_cmd_lcall {
    union {
        struct pisces_lcall lcall;
        struct pisces_lcall_resp lcall_resp;
    } __attribute__((packed));
    char name[128];
    host_pci_cmd_t cmd;
    u64 arg;
} __attribute__((packed));

struct pisces_assigned_dev * pisces_pci_dev_init(struct pisces_pci_dev * device);

int pisces_pci_iommu_map(
    struct pisces_enclave * enclave,
    struct pisces_xbuf_desc * xbuf_desc,
    struct pisces_pci_iommu_map_lcall * lcall);

int pisces_pci_ack_irq(
    struct pisces_enclave * enclave,
    struct pisces_xbuf_desc * xbuf_desc,
    struct pisces_pci_ack_irq_lcall * cur_lcall);

int pisces_pci_cmd(
    struct pisces_enclave * enclave,
    struct pisces_xbuf_desc * xbuf_desc,
    struct pisces_pci_cmd_lcall * cur_lcall);

#endif
