#include <linux/spinlock.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/mutex.h>
#include <linux/kernel.h>
#include <linux/pci.h>

#include "../include/harddoom.h"
#include "../include/doomcode.h"

#include "chrdev.h"
#include "pci.h"
#include "private.h"

#define DRIVER_NAME "harddoom"

struct doom_pci_prv {
  struct doom_prv *drvdata;

  dev_t           dev;
  struct cdev     *cdev;
  struct device   *device;
};

static int load_microcode(void __iomem *BAR0) {
  size_t i;

  iowrite32(0, BAR0 + HARDDOOM_FE_CODE_ADDR);

  for (i = 0; i < ARRAY_SIZE(doomcode); ++i) {
    iowrite32(doomcode[i], BAR0 + HARDDOOM_FE_CODE_WINDOW);
  }

  iowrite32(HARDDOOM_RESET_ALL, BAR0 + HARDDOOM_RESET);
  // Initialize command iomem here (CMD_*_PTR)
  iowrite32(HARDDOOM_INTR_MASK, BAR0 + HARDDOOM_INTR);
  // Enable used interrupts (INTR_ENABLE) here
  iowrite32(HARDDOOM_ENABLE_ALL & ~HARDDOOM_ENABLE_FETCH_CMD, BAR0 + HARDDOOM_ENABLE);

  return 0;
}

static int init_drvdata(struct pci_dev *dev, struct doom_prv **drvdata) {
  unsigned long err;
  struct doom_prv *data;

  data = (struct doom_prv *) kzalloc(sizeof(struct doom_prv), GFP_KERNEL);
  if (IS_ERR(data)) {
    printk(KERN_ERR "[doompci] Init Shared error: kzalloc\n");
    err = PTR_ERR(data);
    goto init_drvdata_kzalloc_err;
  }

  data->BAR0 = pci_iomap(dev, 0, 0);
  if (IS_ERR(data->BAR0)) {
    printk(KERN_ERR "[doompci] Init Shared error: pci_iomap\n");
    err = PTR_ERR(data->BAR0);
    goto init_drvdata_iomap_err;
  }

  data->pci = &dev->dev;

  spin_lock_init(&data->fifo_lock);
  mutex_init(&data->cmd_mutex);
  *drvdata = data;
  return 0;

init_drvdata_iomap_err:
  kfree(data);
init_drvdata_kzalloc_err:
  return err;
}

static void destroy_drvdata(struct pci_dev *dev, struct doom_prv *drvdata) {
  pci_iounmap(dev, drvdata->BAR0);
  kfree(drvdata);
}

static int init_device(struct pci_dev *dev, struct doom_pci_prv *drvdata) {
  unsigned long err;

  drvdata->cdev = doom_cdev_alloc(&drvdata->dev);
  if (IS_ERR(drvdata->cdev)) {
    printk(KERN_ERR "[doompci] Init Device error: doom_cdev_alloc\n");
    err = PTR_ERR(drvdata->cdev);
    goto init_device_cdev_err;
  }

  err = cdev_add(drvdata->cdev, drvdata->dev, 1);
  if (IS_ERR_VALUE(err)) {
    printk(KERN_INFO "[doompci] Init Device error error: cdev_add\n");
    goto init_device_add_err;
  }

  drvdata->device = doom_device_create(&dev->dev, drvdata->drvdata);
  if (IS_ERR(drvdata->device)) {
    printk(KERN_ERR "[doompci] Init Device error: device_create\n");
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

static void destroy_device(struct doom_pci_prv *prv) {
  doom_device_destroy(prv->dev);
  cdev_del(prv->cdev);
}

static int init_private(struct pci_dev *dev) {
  unsigned long err;
  struct doom_pci_prv *prv;

  prv = (struct doom_pci_prv *) kzalloc(sizeof(struct doom_pci_prv), GFP_KERNEL);
  if (IS_ERR(prv)) {
    printk(KERN_ERR "[doompci] Init Private error: kzalloc\n");
    err = PTR_ERR(prv);
    goto init_private_kzalloc_err;
  }

  err = init_drvdata(dev, &prv->drvdata);
  if (IS_ERR_VALUE(err)) {
    printk(KERN_INFO "[doompci] Init Private error: init_drvdata\n");
    goto init_private_shared_err;
  }

  err = init_device(dev, prv);
  if (IS_ERR_VALUE(err)) {
    printk(KERN_INFO "[doompci] Init Private error: init_device\n");
    goto init_private_device_err;
  }

  pci_set_drvdata(dev, prv);
  return 0;

init_private_device_err:
  destroy_drvdata(dev, prv->drvdata);
init_private_shared_err:
  kfree(prv);
init_private_kzalloc_err:
  return err;
}

static void destroy_private(struct pci_dev *dev) {
  struct doom_pci_prv *prv = (struct doom_pci_prv *) pci_get_drvdata(dev);
  destroy_device(prv);
  destroy_drvdata(dev, prv->drvdata);
  kfree(prv);
}

static int probe(struct pci_dev *dev, const struct pci_device_id *id) {
  unsigned long err;
  struct doom_pci_prv *prv;

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

  pci_set_master(dev);

  err = pci_set_dma_mask(dev, DMA_BIT_MASK(32));
  if (IS_ERR_VALUE(err)) {
    printk(KERN_ERR "[doompci] Init Device error: pci_set_dma_mask\n");
    goto probe_dma_err;
  }

  err = pci_set_consistent_dma_mask(dev, DMA_BIT_MASK(32));
  if (IS_ERR_VALUE(err)) {
    printk(KERN_ERR "[doompci] Init Device error: pci_set_consistent_dma_mask\n");
    goto probe_dma_err;
  }

  prv = (struct doom_pci_prv *) pci_get_drvdata(dev);
  load_microcode(prv->drvdata->BAR0);

  printk(KERN_INFO "[doompci] Probe success\n");
  return 0;

probe_dma_err:
  destroy_private(dev);
probe_init_private_err:
  pci_release_regions(dev);
probe_request_err:
  pci_disable_device(dev);
probe_enable_err:
  return err;
}

static void remove(struct pci_dev *dev) {
  pci_clear_master(dev);
  destroy_private(dev);
  pci_release_regions(dev);
  pci_disable_device(dev);
  printk(KERN_INFO "[doompci] Remove finished\n");
}

static const struct pci_device_id pci_ids[] = {
  { PCI_DEVICE(HARDDOOM_VENDOR_ID, HARDDOOM_DEVICE_ID) },
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
