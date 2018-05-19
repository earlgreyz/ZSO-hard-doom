#ifndef HARDDOOM_PRIVATE_H
#define HARDDOOM_PRIVATE_H

#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/pci.h>
#include <linux/spinlock.h>
#include <linux/wait.h>

struct doom_prv {
  void __iomem      *BAR0;

  struct device     *pci;
  struct device     *device;
  struct cdev       cdev;
  dev_t             dev;

  struct mutex      cmd_mutex;
  uint32_t          cmd_idx;
  uint32_t          *cmd;
  dma_addr_t        cmd_dma;

  uint32_t          fifo_count;
  struct semaphore  fifo_wait;
  struct semaphore  fifo_queue;

  wait_queue_head_t fence_wait;
  spinlock_t        fence_lock;
  uint32_t          fence;
  uint32_t          fence_last;

  // Cache for currently set values
  uint32_t          surf_width;
  uint32_t          surf_height;
  dma_addr_t        surf_src;
  dma_addr_t        surf_dst;
  dma_addr_t        texture;
  dma_addr_t        colormap;
  dma_addr_t        translation;
};

#endif
