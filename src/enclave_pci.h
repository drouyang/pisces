/*
 * Device Assignment
 * (c) Jiannan Ouyang, 2013 (ouyang@cs.pitt.edu)
 */

#ifndef _PISCES_PCI_H_
#define _PISCES_PCI_H_

#include "pisces.h"


struct pisces_enclave;
struct pisces_xbuf_desc;

struct pci_iommu_map_lcall;
struct pci_iommu_unmap_lcall;
struct pci_attach_lcall;
struct pci_detach_lcall;
struct pci_ack_irq_lcall;
struct pci_cmd_lcall;





struct enclave_pci_state {
    spinlock_t       lock;
    struct list_head dev_list;
    u32              dev_cnt;
};



struct pisces_pci_dev {
    char name[128];
    u32  domain;
    u32  bus;
    u32  devfn;
    
    struct pisces_enclave * enclave;
    
    u8 ready;
    u8 assigned;

    u8 iommu_enabled;
    struct iommu_domain * iommu_domain;
    
    u32 device_ipi_vector; /* for irq forwarding */
    
    spinlock_t intx_lock;
    u8         intx_disabled;
    
    struct pci_dev    * dev;


    struct list_head dev_node;
};


#ifdef PCI_ENABLED

int 
init_enclave_pci(struct pisces_enclave * enclave);

int 
deinit_enclave_pci(struct pisces_enclave * enclave);

int
enclave_pci_add_dev(struct pisces_enclave  * enclave,
		    struct pisces_pci_spec * spec);

int
enclave_pci_remove_dev(struct pisces_enclave  * enclave,
		       struct pisces_pci_spec * spec);




int 
enclave_pci_iommu_map(struct pisces_enclave      * enclave,
		      struct pisces_xbuf_desc    * xbuf_desc,
		      struct pci_iommu_map_lcall * lcall);

int 
enclave_pci_iommu_unmap(struct pisces_enclave        * enclave,
			struct pisces_xbuf_desc      * xbuf_desc,
			struct pci_iommu_unmap_lcall * lcall);

int 
enclave_pci_attach(struct pisces_enclave   * enclave,
		   struct pisces_xbuf_desc * xbuf_desc,
		   struct pci_attach_lcall * lcall);

int 
enclave_pci_detach(struct pisces_enclave   * enclave,
		   struct pisces_xbuf_desc * xbuf_desc,
		   struct pci_detach_lcall * lcall);

int 
enclave_pci_ack_irq(struct pisces_enclave    * enclave,
		    struct pisces_xbuf_desc  * xbuf_desc,
		    struct pci_ack_irq_lcall * cur_lcall);

int
enclave_pci_cmd(struct pisces_enclave   * enclave,
		struct pisces_xbuf_desc * xbuf_desc,
		struct pci_cmd_lcall    * cur_lcall);



#else 

static inline int 
init_enclave_pci(struct pisces_enclave * enclave) { 
    printk("PCI Support Not enabled for this kernel. Ignoring.\n");
    return -1; 
}

static inline int 
deinit_enclave_pci(struct pisces_enclave * enclave) { 
    printk("PCI Support Not enabled for this kernel. Ignoring.\n");
    return -1; 
}

static inline int
enclave_pci_add_dev(struct pisces_enclave  * enclave,
		    struct pisces_pci_spec * spec) { return -1; }

static inline int
enclave_pci_remove_dev(struct pisces_enclave  * enclave,
		       struct pisces_pci_spec * spec) { return -1; }


#endif


#endif
