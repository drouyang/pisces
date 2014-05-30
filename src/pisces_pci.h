/*
 * Device Assignment
 * (c) Jiannan Ouyang, 2013 (ouyang@cs.pitt.edu)
 */

#ifndef _PISCES_PCI_H_
#define _PISCES_PCI_H_

#include "pisces.h"


struct pisces_pci_state {
    spinlock_t       lock;
    struct list_head dev_list;
    u32              dev_num;
};


struct pisces_enclave;
struct pisces_xbuf_desc;

struct pci_iommu_map_lcall;
struct pci_iommu_unmap_lcall;
struct pci_attach_lcall;
struct pci_detach_lcall;
struct pci_ack_irq_lcall;
struct pci_cmd_lcall;


int 
pisces_pci_init(struct pisces_enclave * enclave);

int
pisces_pci_add_dev(struct pisces_enclave  * enclave,
		   struct pisces_pci_spec * spec);

int
pisces_pci_remove_dev(struct pisces_enclave  * enclave,
		      struct pisces_pci_spec * spec);




int 
pisces_pci_iommu_map(struct pisces_enclave      * enclave,
		     struct pisces_xbuf_desc    * xbuf_desc,
		     struct pci_iommu_map_lcall * lcall);

int 
pisces_pci_iommu_unmap(struct pisces_enclave        * enclave,
		       struct pisces_xbuf_desc      * xbuf_desc,
		       struct pci_iommu_unmap_lcall * lcall);

int 
pisces_pci_attach(struct pisces_enclave   * enclave,
		  struct pisces_xbuf_desc * xbuf_desc,
		  struct pci_attach_lcall * lcall);

int 
pisces_pci_detach(struct pisces_enclave   * enclave,
		  struct pisces_xbuf_desc * xbuf_desc,
		  struct pci_detach_lcall * lcall);

int 
pisces_pci_ack_irq(struct pisces_enclave    * enclave,
		   struct pisces_xbuf_desc  * xbuf_desc,
		   struct pci_ack_irq_lcall * cur_lcall);

int
pisces_pci_cmd(struct pisces_enclave   * enclave,
	       struct pisces_xbuf_desc * xbuf_desc,
	       struct pci_cmd_lcall    * cur_lcall);



#endif
