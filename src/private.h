#ifndef HARDDOOM_PRIVATE_H
#define HARDDOOM_PRIVATE_H

#include <linux/spinlock.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/pci.h>

struct doom_prv {
  void __iomem    *BAR0;

  spinlock_t      lock;
  uint32_t        fence_count;

  dev_t           dev;
  struct cdev     *cdev;
  struct device   *device;
};

#endif
