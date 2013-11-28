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
  PT_BAR_NONE,
  PT_BAR_IO,
  PT_BAR_MEM32,
  PT_BAR_MEM24,
  PT_BAR_MEM64_LO,
  PT_BAR_MEM64_HI,
  PT_EXP_ROM
} pt_bar_type_t;


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

struct v3_host_pci_bar {
  uint32_t size;
  pt_bar_type_t type;

  /*  We store 64 bit memory bar addresses in the high BAR
   *  because they are the last to be updated
   *  This means that the addr field must be 64 bits
   */
  uint64_t addr;

  union {
    uint32_t flags;
    struct {
      uint32_t prefetchable    : 1;
      uint32_t cacheable       : 1;
      uint32_t exp_rom_enabled : 1;
      uint32_t rsvd            : 29;
    } __attribute__((packed));
  } __attribute__((packed));
};

struct pisces_pci_setup_lcall {
    struct pisces_lcall lcall; 
    char name[128];
} __attribute__((packed));


struct pisces_pci_setup_resp {
    struct pisces_lcall_resp resp;
    u32 domain;
    u32 bus;
    u32 devfn;
    u64 iommu_present;
    struct v3_host_pci_bar bars[6];
    struct v3_host_pci_bar exp_rom;
    uint8_t cfg_space[256];
} __attribute__((packed));

int pisces_device_assign(struct pisces_host_pci_bdf * bdf);
int enclave_pci_setup_lcall(struct pisces_enclave * enclave, 
			    struct pisces_xbuf_desc * xbuf_desc,
                            struct pisces_lcall * lcall);
int enclave_pci_request_deivce_lcall(struct pisces_enclave * enclave,
				     struct pisces_xbuf_desc * xbuf_desc,
                                     struct pisces_lcall * lcall);

#endif
