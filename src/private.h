#ifndef HARDDOOM_PRIVATE_H
#define HARDDOOM_PRIVATE_H

#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/pci.h>
#include <linux/spinlock.h>
#include <linux/wait.h>

/// Shared among all of the objects with access to the device.
struct doom_prv {
  void __iomem      *BAR0;        // Address of the BAR0 on the harddoom device.

  // Device data
  struct device     *pci;
  struct device     *device;
  struct cdev       cdev;
  dev_t             dev;

  // Commands
  uint32_t          *cmd;         // Commands block address.
  dma_addr_t        cmd_dma;      // Commands block DMA address.
  uint32_t          cmd_idx;      // Current index in the commands block.
  struct mutex      cmd_mutex;    // Mutex to claim when sending a block of cmd.

  uint8_t           fifo_free;    // Number of commands which can be sent immidately without calling cmd_wait.
  uint32_t          fifo_count;   // Number of commands after which a PING_ASYNC will be sent.
  struct semaphore  fifo_wait;    // Semaphore to wait in case the fifo is full.

  wait_queue_head_t fence_wait;   // Wait queue for process before a FENCE interrupt.
  spinlock_t        fence_lock;   // Spinlock for changing the FENCE counter.
  uint32_t          fence;        // Fence counter.
  uint32_t          fence_last;   // Last FENCE, which has been completed.
  struct mutex      read_mutex;   // Mutex for read.

  // Cache for currently set values
  uint32_t          surf_width;   // Currently selected surface width.
  uint32_t          surf_height;  // Currently selected surface height.
  dma_addr_t        surf_src;     // Currently selected surface source.
  dma_addr_t        surf_dst;     // Currently selected surface destination.
  dma_addr_t        texture;      // Currently selected texture.
  dma_addr_t        colormap;     // Currently selected colormap.
  dma_addr_t        translation;  // Currently selected translation colormap.
};

#endif
