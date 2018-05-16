#include <linux/spinlock.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/mutex.h>
#include <linux/pci.h>
#include <linux/semaphore.h>

#include "../include/harddoom.h"
#include "../include/doomcode.h"

#include "chrdev.h"
#include "pci.h"
#include "private.h"

#define DRIVER_NAME "harddoom"

static int load_microcode(void __iomem *BAR0) {
  size_t i;

  iowrite32(0, BAR0 + HARDDOOM_FE_CODE_ADDR);

  for (i = 0; i < ARRAY_SIZE(doomcode); ++i) {
    iowrite32(doomcode[i], BAR0 + HARDDOOM_FE_CODE_WINDOW);
  }

  iowrite32(HARDDOOM_RESET_ALL, BAR0 + HARDDOOM_RESET);
  // Initialize command iomem here (CMD_*_PTR)
  iowrite32(HARDDOOM_INTR_MASK, BAR0 + HARDDOOM_INTR);
  iowrite32(HARDDOOM_INTR_MASK, BAR0 + HARDDOOM_INTR_ENABLE);
  iowrite32(HARDDOOM_ENABLE_ALL & ~HARDDOOM_ENABLE_FETCH_CMD, BAR0 + HARDDOOM_ENABLE);

  return 0;
}

static irqreturn_t irq_handler(int irq, void *dev) {
  struct doom_prv *drvdata = (struct doom_prv *) dev;
  unsigned int interrupts;

  interrupts = ioread32(drvdata->BAR0 + HARDDOOM_INTR);
  if (interrupts == 0x00) {
    return IRQ_NONE;
  }

  iowrite32(interrupts, drvdata->BAR0 + HARDDOOM_INTR);

  if (interrupts & HARDDOOM_INTR_PONG_SYNC) {
    up(&drvdata->ping_wait);
    up(&drvdata->ping_queue);
  } else if (interrupts & HARDDOOM_INTR_FE_ERROR) {
    printk(KERN_INFO "[doomirq] FE_ERROR %x for command %x\n", \
      ioread32(drvdata->BAR0 + HARDDOOM_FE_ERROR_CODE), \
      ioread32(drvdata->BAR0 + HARDDOOM_FE_ERROR_CMD));
  } else {
    printk(KERN_INFO "[doomirq] Received interrupt %x\n", interrupts);
  }

  return IRQ_HANDLED;
}

static int init_device(struct pci_dev *dev, struct doom_prv *drvdata) {
  unsigned long err;
  doom_cdev_init(&drvdata->cdev, &drvdata->dev);

  err = cdev_add(&drvdata->cdev, drvdata->dev, 1);
  if (IS_ERR_VALUE(err)) {
    printk(KERN_INFO "[doompci] Init Device error error: cdev_add\n");
    goto init_device_add_err;
  }

  drvdata->device = doom_device_create(&dev->dev, drvdata);
  if (IS_ERR(drvdata->device)) {
    printk(KERN_ERR "[doompci] Init Device error: device_create\n");
    err = PTR_ERR(drvdata->device);
    goto init_device_create_err;
  }

  return 0;

init_device_create_err:
init_device_add_err:
  cdev_del(&drvdata->cdev);
  return err;
}

static void destroy_device(struct doom_prv *drvdata) {
  doom_device_destroy(drvdata->dev);
  cdev_del(&drvdata->cdev);
}

static int init_drvdata(struct pci_dev *dev) {
  unsigned long err;
  struct doom_prv *drvdata;

  drvdata = (struct doom_prv *) kzalloc(sizeof(struct doom_prv), GFP_KERNEL);
  if (IS_ERR(drvdata)) {
    printk(KERN_ERR "[doompci] Init drvdata error: kzalloc\n");
    err = PTR_ERR(drvdata);
    goto init_private_kzalloc_err;
  }

  drvdata->BAR0 = pci_iomap(dev, 0, 0);
  if (IS_ERR(drvdata->BAR0)) {
    printk(KERN_ERR "[doompci] Init Shared error: pci_iomap\n");
    err = PTR_ERR(drvdata->BAR0);
    goto init_drvdata_iomap_err;
  }

  drvdata->pci = &dev->dev;

  spin_lock_init(&drvdata->fifo_lock);
  mutex_init(&drvdata->cmd_mutex);
  sema_init(&drvdata->ping_wait, 1);
  sema_init(&drvdata->ping_queue, 1);

  err = init_device(dev, drvdata);
  if (IS_ERR_VALUE(err)) {
    printk(KERN_INFO "[doompci] Init drvdata error: init_device\n");
    goto init_drvdata_device_err;
  }

  pci_set_drvdata(dev, drvdata);
  return 0;

init_drvdata_device_err:
  pci_iounmap(dev, drvdata->BAR0);
init_drvdata_iomap_err:
  kfree(drvdata);
init_private_kzalloc_err:
  return err;
}

static void destroy_drvdata(struct pci_dev *dev) {
  struct doom_prv *drvdata = (struct doom_prv *) pci_get_drvdata(dev);
  destroy_device(drvdata);
  pci_iounmap(dev, drvdata->BAR0);
  kfree(drvdata);
}

static int probe(struct pci_dev *dev, const struct pci_device_id *id) {
  unsigned long err;
  struct doom_prv *drvdata;

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

  err = init_drvdata(dev);
  if (IS_ERR_VALUE(err)) {
    printk(KERN_ERR "[doompci] Probe error: init_private\n");
    goto probe_init_drvdata_err;
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

  drvdata = (struct doom_prv *) pci_get_drvdata(dev);

  err = request_irq(dev->irq, &irq_handler, IRQF_SHARED, "doom", drvdata);
  if (IS_ERR_VALUE(err)) {
    printk(KERN_ERR "[doompci] Init Device error: request_irq\n");
    goto probe_irq_err;
  }

  load_microcode(drvdata->BAR0);

  printk(KERN_INFO "[doompci] Probe success\n");
  return 0;

probe_irq_err:
  pci_clear_master(dev);
probe_dma_err:
  destroy_drvdata(dev);
probe_init_drvdata_err:
  pci_release_regions(dev);
probe_request_err:
  pci_disable_device(dev);
probe_enable_err:
  return err;
}

static void remove(struct pci_dev *dev) {
  struct doom_prv *drvdata = pci_get_drvdata(dev);
  free_irq(dev->irq, drvdata);
  pci_clear_master(dev);
  destroy_drvdata(dev);
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
