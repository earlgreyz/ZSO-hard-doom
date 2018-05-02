#include <linux/kernel.h>

#include "doompci.h"
#include "doompci/device.h"
#include "doompci/doomcode.h"
#include "doompci/registers.h"

static int load_microcode(struct pci_dev *dev) {
  void __iomem *BAR0 = pci_iomap(dev, 0, 0);
  if (IS_ERR(BAR0)) {
    return PTR_ERR(BAR0);
  }

  iowrite32(BAR0 + DOOMPCI_FE_CODE_ADDR, 0);

  for (size_t i = 0; i < ARRAY_SIZE(doomcode); ++i) {
    iowrite32(BAR0 + DOOMPCI_FE_CODE_WINDOW, doomcode[i]);
  }

  iowrite32(BAR0 + DOOMPCI_RESET, RESET_ALL);
  // Initialize command iomem here (CMD_*_PTR)
  iowrite32(BAR0 + DOOMPCI_INTR, INTR_ALL);
  // Enable used interrupts (INTR_ENABLE) here
  iowrite32(BAR0 + DOOMPCI_ENABLE, ENABLE_ALL & ~ENABLE_FETCH)

  pci_iounmap(dev, BAR0);
  return 0;
}

static int probe(struct pci_dev *dev, const struct pci_device_id *id) {
  int err;
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

  err = load_microcode(dev);
  if (IS_ERR(err)) {
    printk(KERN_ERR "Probe error: load_microcode\n");
    goto probe_microcode_err;
  }

  printk(KERN_INFO "Probe finished\n");
  return 0;

probe_microcode_err:
  pci_release_regions(dev);
probe_request_err:
  pci_disable_device(dev);
probe_enable_err:
  return err;
}

static void remove(struct pci_dev *dev) {
  printk(KERN_INFO "Remove started\n");
  pci_release_regions(dev);
  pci_disable_device(dev);
  printk(KERN_INFO "Remove finished\n");
}

static const struct pci_device_id pci_ids[] = {
  PCI_DEVICE(DOOMPCI_VENDOR_ID, DOOMPCI_DEVICE_ID),
  {},
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
