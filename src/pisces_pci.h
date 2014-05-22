/*
 * Device Assignment
 * (c) Jiannan Ouyang, 2013 (ouyang@cs.pitt.edu)
 */

#ifndef _PISCES_PCI_H_
#define _PISCES_PCI_H_

#include "pisces.h"
#include "enclave.h"
#include "pisces_cmds.h"

struct pisces_pci_dev {
    char name[128];
    u32  domain;
    u32  bus;
    u32  devfn;
    
    struct pisces_enclave * enclave;
    
    struct list_head dev_node;
    
    u8 in_use;
    u8 assigned;

    u8 iommu_enabled;
    struct iommu_domain * iommu_domain;
    
    u32 device_ipi_vector; /* for irq forwarding */
    
    spinlock_t intx_lock;
    u8         intx_disabled;
    
    struct pci_dev    * dev;
};


/* Palacios Specific Definations */

typedef enum {
    HOST_PCI_CMD_DMA_DISABLE  = 1,
    HOST_PCI_CMD_DMA_ENABLE   = 2,
    HOST_PCI_CMD_MEM_ENABLE   = 3,
    HOST_PCI_CMD_INTX_DISABLE = 4,
    HOST_PCI_CMD_INTX_ENABLE  = 5,
} host_pci_cmd_t;

struct pci_iommu_map_lcall {
    struct pisces_lcall      lcall;

    char name[128];
    u64  region_start;
    u64  region_end;
    u64  gpa;
} __attribute__((packed));


struct pci_attach_lcall {
    struct pisces_lcall      lcall;

    char name[128];
    u32  ipi_vector;
} __attribute__((packed));



struct pci_ack_irq_lcall {
    struct pisces_lcall      lcall;

    char name[128];
    u32  vector;
} __attribute__((packed));


struct pci_cmd_lcall {
    struct pisces_lcall      lcall;

    char           name[128];
    host_pci_cmd_t cmd;
    u64            arg;
} __attribute__((packed));


int
pisces_pci_dev_init(struct pisces_enclave  * enclave,
		    struct pisces_pci_spec * spec);

int 
pisces_pci_iommu_map(struct pisces_enclave      * enclave,
		     struct pisces_xbuf_desc    * xbuf_desc,
		     struct pci_iommu_map_lcall * lcall);
int 
pisces_pci_attach(struct pisces_enclave   * enclave,
		  struct pisces_xbuf_desc * xbuf_desc,
		  struct pci_attach_lcall * lcall);

int 
pisces_pci_ack_irq(struct pisces_enclave    * enclave,
		   struct pisces_xbuf_desc  * xbuf_desc,
		   struct pci_ack_irq_lcall * cur_lcall);

int
pisces_pci_cmd(struct pisces_enclave   * enclave,
	       struct pisces_xbuf_desc * xbuf_desc,
	       struct pci_cmd_lcall    * cur_lcall);



#endif
