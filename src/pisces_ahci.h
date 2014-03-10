#ifndef _PISCES_AHCI_H_
#define _PISCES_AHCI_H_

//#define DEBUG_AHCI

#define PISCES_AHCI_IPI_VECTOR  221

struct pisces_host_sata_dev {
  u32 domain;
  u32 bus;
  u32 devfn;
  u32 irq;
  u32 bar_id; /* usually it's 5 */
  u32 port;
};


int pisces_ahci_init(struct pisces_host_sata_dev *assigned_dev);

#endif
