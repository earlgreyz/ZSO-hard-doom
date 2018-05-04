#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/device.h>
#include <linux/cdev.h>

#include "pci.h"
#include "chrdev.h"
#include "../include/doompci.h"
#include "../include/doomreg.h"
#include "../include/doomcode.h"

struct doom_pci_drvdata {
  void __iomem *BAR0;
  dev_t dev;
  struct cdev *cdev;
  struct device *device;
};

static int load_microcode(struct pci_dev *dev) {
  size_t i;
  struct doom_pci_drvdata *drvdata = pci_get_drvdata(dev);

  iowrite32(0, drvdata->BAR0 + DOOMREG_FE_CODE_ADDR);

  for (i = 0; i < ARRAY_SIZE(doomcode); ++i) {
    iowrite32(doomcode[i], drvdata->BAR0 + DOOMREG_FE_CODE_WINDOW);
  }

  iowrite32(RESET_ALL, drvdata->BAR0 + DOOMREG_RESET);
  // Initialize command iomem here (CMD_*_PTR)
  iowrite32(INTR_ALL, drvdata->BAR0 + DOOMREG_INTR);
  // Enable used interrupts (INTR_ENABLE) here
  iowrite32(ENABLE_ALL & ~ENABLE_FETCH, drvdata->BAR0 + DOOMREG_ENABLE);

  return 0;
}

static int probe(struct pci_dev *dev, const struct pci_device_id *id) {
  unsigned long err;
  struct doom_pci_drvdata *drvdata;

  err = pci_enable_device(dev);
  if (IS_ERR_VALUE(err)) {
    printk(KERN_ERR "[doompci] Probe error: pci_enable_device\n");
    goto probe_enable_err;
  }

  err = pci_request_regions(dev, "doompci");
  if (IS_ERR_VALUE(err)) {
    printk(KERN_ERR "[doompci] Probe error: pci_request_regions\n");
    goto probe_request_err;
  }

  drvdata = (struct doom_pci_drvdata *) kmalloc(sizeof(struct doom_pci_drvdata), GFP_KERNEL);
  if (IS_ERR(drvdata)) {
    printk(KERN_ERR "[doompci] Probe error: kmalloc\n");
    err = PTR_ERR(drvdata);
    goto probe_kmalloc_err;
  }

  pci_set_drvdata(dev, drvdata);

  drvdata->BAR0 = pci_iomap(dev, 0, 0);
  if (IS_ERR(drvdata->BAR0)) {
    printk(KERN_INFO "[doompci] Probe error: pci_iomap\n");
    err = PTR_ERR(drvdata->BAR0);
    goto probe_iomap_err;
  }

  drvdata->cdev = doom_cdev_alloc();
  if (IS_ERR(drvdata->cdev)) {
    printk(KERN_ERR "[doompci] Probe error: doom_cdev_alloc\n");
    err = PTR_ERR(drvdata->cdev);
    goto probe_cdev_alloc_err;
  }

  drvdata->device = doom_device_create(&dev->dev);
  if (IS_ERR(drvdata->device)) {
    printk(KERN_ERR "[doompci] Probe error: device_create\n");
    err = PTR_ERR(drvdata->device);
    goto probe_device_create_err;
  }

  load_microcode(dev);
  printk(KERN_INFO "[doompci] Probe success\n");
  return 0;

probe_device_create_err:
  cdev_del(drvdata->cdev);
probe_cdev_alloc_err:
  pci_iounmap(dev, drvdata->BAR0);
probe_iomap_err:
  kfree(drvdata);
probe_kmalloc_err:
  pci_release_regions(dev);
probe_request_err:
  pci_disable_device(dev);
probe_enable_err:
  return err;
}

static void remove(struct pci_dev *dev) {
  struct doom_pci_drvdata *drvdata = pci_get_drvdata(dev);
  doom_device_destroy(drvdata->device);
  cdev_del(drvdata->cdev);
  pci_iounmap(dev, drvdata->BAR0);
  kfree(drvdata);
  pci_release_regions(dev);
  pci_disable_device(dev);
  printk(KERN_INFO "[doompci] Remove finished\n");
}

static const struct pci_device_id pci_ids[] = {
  { PCI_DEVICE(DOOMPCI_VENDOR_ID, DOOMPCI_DEVICE_ID) },
  { },
};

static struct pci_driver pci_driver = {
  .name = "harddoom",
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
