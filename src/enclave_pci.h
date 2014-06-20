/*
 * Device Assignment
 * (c) Jiannan Ouyang, 2013 (ouyang@cs.pitt.edu)
 */

#ifndef _PISCES_PCI_H_
#define _PISCES_PCI_H_

#include "pisces.h"


struct enclave_pci_state {
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



#endif
