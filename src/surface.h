#ifndef HARDDOOM_SURFACE_H
#define HARDDOOM_SURFACE_H

#include <linux/kernel.h>
#include <linux/file.h>

#include "../include/doomdev.h"

#include "private.h"

struct surface_prv {
  struct doom_prv   *drvdata;

  uint32_t          width;
  uint32_t          height;

  size_t            size;
  void              *surface;
  struct pt_entry   *pt;

  dma_addr_t        surface_dma;
  dma_addr_t        pt_dma;
};

long surface_create(struct doom_prv *drvdata, struct doomdev_ioctl_create_surface *args);
bool is_surface_fd(struct fd *fd);

#endif
