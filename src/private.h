#ifndef HARDDOOM_PRIVATE_H
#define HARDDOOM_PRIVATE_H

#include <linux/spinlock.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/pci.h>

struct doom_prv {
  void __iomem     *BAR0;

  struct device    *pci;
  struct device    *device;
  struct cdev      cdev;
  dev_t            dev;

  spinlock_t       fifo_lock;
  struct mutex     cmd_mutex;

  struct semaphore ping_wait;
  struct semaphore ping_queue;

  // Cache for currently set values
  dma_addr_t       surf_src;
  dma_addr_t       surf_dst;
  uint32_t         surf_width;
  uint32_t         surf_height;
  dma_addr_t       texture;
};

#endif
