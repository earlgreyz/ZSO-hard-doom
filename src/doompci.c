#include <linux/kernel.h>
#include <linux/pci.h>

#include "doompci.h"
#include "doompci/device.h"
#include "doompci/doomcode.h"
#include "doompci/registers.h"

static int load_microcode(struct pci_dev *dev) {
  size_t i;
  void __iomem *BAR0;

  BAR0 = pci_get_drvdata(dev);

  iowrite32(0, BAR0 + DOOMPCI_FE_CODE_ADDR);

  for (i = 0; i < ARRAY_SIZE(doomcode); ++i) {
    iowrite32(doomcode[i], BAR0 + DOOMPCI_FE_CODE_WINDOW);
  }

  iowrite32(RESET_ALL, BAR0 + DOOMPCI_RESET);
  // Initialize command iomem here (CMD_*_PTR)
  iowrite32(INTR_ALL, BAR0 + DOOMPCI_INTR);
  // Enable used interrupts (INTR_ENABLE) here
  iowrite32(ENABLE_ALL & ~ENABLE_FETCH, BAR0 + DOOMPCI_ENABLE);

  return 0;
}

static int probe(struct pci_dev *dev, const struct pci_device_id *id) {
  unsigned long err;
  void __iomem *BAR0;

  printk(KERN_INFO "Probe started\n");

  err = pci_enable_device(dev);
  if (IS_ERR_VALUE(err)) {
    printk(KERN_ERR "Probe error: pci_enable_device\n");
    goto probe_enable_err;
  }

  err = pci_request_regions(dev, "doompci");
  if (IS_ERR_VALUE(err)) {
    printk(KERN_ERR "Probe error: pci_request_regions\n");
    goto probe_request_err;
  }

  BAR0 = pci_iomap(dev, 0, 0);
  if (IS_ERR(BAR0)) {
    printk(KERN_INFO "Probe error: pci_iomap\n");
    err = PTR_ERR(BAR0);
    goto probe_iomap_err;
  }

  pci_set_drvdata(dev, BAR0);
  load_microcode(dev);

  printk(KERN_INFO "Probe finished\n");
  return 0;

probe_iomap_err:
  pci_release_regions(dev);
probe_request_err:
  pci_disable_device(dev);
probe_enable_err:
  return err;
}

static void remove(struct pci_dev *dev) {
  printk(KERN_INFO "Remove\n");
  pci_iounmap(dev, pci_get_drvdata(dev));
  pci_release_regions(dev);
  pci_disable_device(dev);
}

static const struct pci_device_id pci_ids[] = {
  { PCI_DEVICE(DOOMPCI_VENDOR_ID, DOOMPCI_DEVICE_ID) },
  { },
};

static struct pci_driver pci_driver = {
  .name = "doom pci",
  .id_table = pci_ids,
  .probe = probe,
  .remove = remove,
};

int doom_pci_register_driver(void) {
  return pci_register_driver(&pci_driver);
}

void doom_pci_unregister_driver(void) {
  pci_unregister_driver(&pci_driver);
}
