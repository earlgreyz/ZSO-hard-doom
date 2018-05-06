#include <linux/spinlock.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/pci.h>

#include "../include/doomcode.h"
#include "../include/doompci.h"
#include "../include/doomreg.h"

#include "chrdev.h"
#include "pci.h"
#include "private.h"

#define DRIVER_NAME "harddoom"

struct doom_pci_prv {
  struct doom_prv *shared_data;

  dev_t           dev;
  struct cdev     *cdev;
  struct device   *device;
};

static int load_microcode(void __iomem *BAR0) {
  size_t i;

  iowrite32(0, BAR0 + DOOMREG_FE_CODE_ADDR);

  for (i = 0; i < ARRAY_SIZE(doomcode); ++i) {
    iowrite32(doomcode[i], BAR0 + DOOMREG_FE_CODE_WINDOW);
  }

  iowrite32(RESET_ALL, BAR0 + DOOMREG_RESET);
  // Initialize command iomem here (CMD_*_PTR)
  iowrite32(INTR_ALL, BAR0 + DOOMREG_INTR);
  // Enable used interrupts (INTR_ENABLE) here
  iowrite32(ENABLE_ALL & ~ENABLE_FETCH, BAR0 + DOOMREG_ENABLE);

  return 0;
}

static int init_shared(struct pci_dev *dev, struct doom_prv **shared_data) {
  unsigned long err;
  struct doom_prv *data;

  data = (struct doom_prv *) kzalloc(sizeof(struct doom_prv), GFP_KERNEL);
  if (IS_ERR(data)) {
    printk(KERN_ERR "[doompci] Init Shared error: kzalloc\n");
    err = PTR_ERR(data);
    goto init_shared_kzalloc_err;
  }

  data->BAR0 = pci_iomap(dev, 0, 0);
  if (IS_ERR(data->BAR0)) {
    printk(KERN_INFO "[doompci] Init Shared error: pci_iomap\n");
    err = PTR_ERR(data->BAR0);
    goto init_shared_iomap_err;
  }

  spin_lock_init(&data->lock);
  *shared_data = data;
  return 0;

init_shared_iomap_err:
  kfree(data);
init_shared_kzalloc_err:
  return err;
}

static void destroy_shared(struct pci_dev *dev, struct doom_prv *shared_data) {
  pci_iounmap(dev, shared_data->BAR0);
  kfree(shared_data);
}

static int init_device(struct pci_dev *dev, struct doom_pci_prv *drvdata) {
  unsigned long err;

  drvdata->cdev = doom_cdev_alloc(&drvdata->dev);
  if (IS_ERR(drvdata->cdev)) {
    printk(KERN_ERR "[doompci] Init Private error: doom_cdev_alloc\n");
    err = PTR_ERR(drvdata->cdev);
    goto init_device_cdev_err;
  }

  err = cdev_add(drvdata->cdev, drvdata->dev, 1);
  if (IS_ERR_VALUE(err)) {
    printk(KERN_INFO "[doompci] Init Private error error: cdev_add\n");
    goto init_device_add_err;
  }

  drvdata->device = doom_device_create(&dev->dev, drvdata->shared_data);
  if (IS_ERR(drvdata->device)) {
    printk(KERN_ERR "[doompci] Init Private error: device_create\n");
    err = PTR_ERR(drvdata->device);
    goto init_device_create_err;
  }

  return 0;

init_device_create_err:
init_device_add_err:
  cdev_del(drvdata->cdev);
init_device_cdev_err:
  return err;
}

static void destroy_device(struct doom_pci_prv *drvdata) {
  doom_device_destroy(drvdata->dev);
  cdev_del(drvdata->cdev);
}

static int init_private(struct pci_dev *dev) {
  unsigned long err;
  struct doom_pci_prv *drvdata;

  drvdata = (struct doom_pci_prv *) kzalloc(sizeof(struct doom_pci_prv), GFP_KERNEL);
  if (IS_ERR(drvdata)) {
    printk(KERN_ERR "[doompci] Init Private error: kzalloc\n");
    err = PTR_ERR(drvdata);
    goto init_private_kzalloc_err;
  }

  err = init_shared(dev, &drvdata->shared_data);
  if (IS_ERR_VALUE(err)) {
    printk(KERN_INFO "[doompci] Init Private error: init_shared\n");
    goto init_private_shared_err;
  }

  err = init_device(dev, drvdata);
  if (IS_ERR_VALUE(err)) {
    printk(KERN_INFO "[doompci] Init Private error: init_device\n");
    goto init_private_device_err;
  }

  pci_set_drvdata(dev, drvdata);
  return 0;

init_private_device_err:
  destroy_shared(dev, drvdata->shared_data);
init_private_shared_err:
  kfree(drvdata);
init_private_kzalloc_err:
  return err;
}

static void destroy_private(struct pci_dev *dev) {
  struct doom_pci_prv *drvdata = (struct doom_pci_prv *) pci_get_drvdata(dev);
  destroy_device(drvdata);
  destroy_shared(dev, drvdata->shared_data);
  kfree(drvdata);
}

static int probe(struct pci_dev *dev, const struct pci_device_id *id) {
  unsigned long err;
  struct doom_pci_prv *drvdata;

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

  err = init_private(dev);
  if (IS_ERR_VALUE(err)) {
    printk(KERN_ERR "[doompci] Probe error: init_private\n");
    goto probe_init_private_err;
  }

  drvdata = (struct doom_pci_prv *) pci_get_drvdata(dev);
  load_microcode(drvdata->shared_data->BAR0);

  printk(KERN_INFO "[doompci] Probe success\n");
  return 0;

probe_init_private_err:
  pci_release_regions(dev);
probe_request_err:
  pci_disable_device(dev);
probe_enable_err:
  return err;
}

static void remove(struct pci_dev *dev) {
  destroy_private(dev);
  pci_release_regions(dev);
  pci_disable_device(dev);
  printk(KERN_INFO "[doompci] Remove finished\n");
}

static const struct pci_device_id pci_ids[] = {
  { PCI_DEVICE(DOOMPCI_VENDOR_ID, DOOMPCI_DEVICE_ID) },
  { },
};

static struct pci_driver pci_driver = {
  .name = DRIVER_NAME,
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
