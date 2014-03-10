/* AHCI SATA Controller Support 
 * IRQ forwarding for single MSI vector
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/pci.h>


#include "pisces_ahci.h"

#ifdef DEBUG_AHCI
#define debug(fmt, args...) printk(KERN_INFO "%s: " fmt, __func__, ## args)
#else 
#define debug(fmt, args...)
#endif

#define MAX_PORTS     4
#define HOST_IRQ_STAT 0x08

struct pisces_sata_dev {
  u32 domain;
  u32 bus;
  u32 devfn;
  u32 irq;
  u32 bar_id; /* usually it's 5 */
  u32 port;

  struct pci_dev * pdev;
  u64 bar_region_pa;
  u64 bar_region_size;
  u64 bar_region_flags;
  u64 flags;
  void __iomem *mmio; /* mapped bar region */

  int ipi_vector;
};

// can be a hook list
struct pisces_sata_dev * sata_dev;

/*
 * Return IRQ_NONE if IRQ does not belong to our driver
 * else forward and return IRQ_HANDLED
 */
//static irqreturn_t ahci_irq_handler(u32 irq_mask)
static void ahci_irq_handler(u32 irq_mask)
{
  //struct pisces_sata_dev *sata_dev = dev_id;
  //void __iomem *mmio;
  //u32 irq_stat;
  int i;
  static u64 port_irq_stats[MAX_PORTS];
  static u64 counter = 0;
  counter++;

  /* if irq is from target device, handle it, return IRQ_HANDLED;
   * else return IRQ_NONE
   */
  //mmio = sata_dev->mmio;

  //irq_stat = readl(mmio + HOST_IRQ_STAT);

  if (!irq_mask) {
    return;
  }

  for (i = 0; i < MAX_PORTS; i++) {
    if (irq_mask & (1 << i)) {
      port_irq_stats[i]++;
    }
  }

  if (counter % 50 == 0) {
    printk("irq stat: %llu %llu %llu %llu\n", 
        (unsigned long long) port_irq_stats[0],
        (unsigned long long) port_irq_stats[1],
        (unsigned long long) port_irq_stats[2],
        (unsigned long long) port_irq_stats[3]);
  }



#if 0
  {
    static int flag = 1;

    if (flag) {
      printk("pisces: ahci_irq_handler, irq %d, priv_data %p, sata_dev->irq %d, sata_dev->port %d\n", 
          irq, sata_dev, sata_dev->irq, sata_dev->port);
      dump_stack();
      flag = 0;
    }
  }
#endif

  return;
}

/*
static int ahci_hook_irq(struct pisces_sata_dev *sata_dev)
{
  int rc;

  if (sata_dev == NULL) {
    printk(KERN_ERR "Error: hook_ahci_irq: null sata_dev pointer\n");
    return -1;
  }

  debug("pisces_ahci: hook irq %d for AHCI port %d\n", sata_dev->irq, sata_dev->port);
  rc = request_threaded_irq(sata_dev->irq, ahci_irq_handler, NULL, IRQF_SHARED,
      "pisces-ahci", sata_dev);

  if (rc) {
    printk(KERN_ERR "Error: hook_ahci_irq: failed to request irq %d, error code %d\n", sata_dev->irq, rc);
    return -1;
  }

  return 0;
}
*/

/* Linux AHCI IRQ handler clears hardware pending irq register before return,
 * which prevents IRQ sharing. So I added a hook into Linux ahci_interrupt() that 
 * is called if not NULL.
 */
extern void (*ahci_irq_hook)(u32);
/* AHCI IRQ forwarding 
 * ref: drivers/ata/ahci.c
 */
int pisces_ahci_init(struct pisces_host_sata_dev *assigned_dev)
{
  struct pci_dev * pdev;
  int ret = 0;

  if (assigned_dev == NULL) {
#ifdef DEBUG_AHCI
    assigned_dev = kmalloc(sizeof(struct pisces_host_sata_dev), GFP_KERNEL);
    assigned_dev->domain = 0;
    assigned_dev->bus = 2;
    assigned_dev->devfn = PCI_DEVFN(2, 0);
    assigned_dev->irq = 72;
    assigned_dev->bar_id = 5;
    assigned_dev->port = 2;
#else
    printk("Error: pisces_ahci_init: null assigned_dev pointer\n");
    return -1;
#endif
  }

  pdev = pci_get_bus_and_slot(
      assigned_dev->bus,
      assigned_dev->devfn);

  if (!pdev) {
    printk(KERN_ERR "pisces_ahci_init: assigned device not found\n");
    ret = -EINVAL;
    goto out;
  }


  sata_dev = kmalloc(sizeof(struct pisces_sata_dev), GFP_KERNEL);
  if (sata_dev == NULL) {
    printk(KERN_ERR "Error: failed to allocated sata_dev\n");
    ret = -1;
    goto out;
  }

  sata_dev->domain = assigned_dev->domain;
  sata_dev->bus = assigned_dev->bus;
  sata_dev->devfn = assigned_dev->devfn;
  sata_dev->irq = assigned_dev->irq;

  sata_dev->pdev = pdev;
  sata_dev->bar_id = assigned_dev->bar_id;
  sata_dev->bar_region_pa = pci_resource_start(pdev, sata_dev->bar_id);
  sata_dev->bar_region_size = pci_resource_len(pdev, sata_dev->bar_id);
  sata_dev->flags = pci_resource_flags(pdev, sata_dev->bar_id);
  sata_dev->mmio = pcim_iomap_table(pdev)[sata_dev->bar_id];
  debug("AHCI bar region pa %p, size %p\n", 
      (void *) sata_dev->bar_region_pa, (void *) sata_dev->bar_region_size);


  sata_dev->port = assigned_dev->port;
  sata_dev->ipi_vector = PISCES_AHCI_IPI_VECTOR;

  //return ahci_hook_irq(sata_dev);
  ahci_irq_hook = ahci_irq_handler;
  return 0;

out:
  return ret;
}

/*
static void ahci_irq_unhook(struct pisces_sata_dev *sata_dev)
{
  free_irq(sata_dev->irq, sata_dev);
}

int pisces_ahci_deinit(struct pisces_host_sata_dev * sata_dev)
{
  ahci_irq_unhook(sata_dev);
  kfree(sata_dev);
  return 0;
}
*/
