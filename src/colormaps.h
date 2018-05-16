#ifndef DOOM_COLORMAP_H
#define DOOM_COLORMAP_H

#include <linux/kernel.h>

#include "../include/doomdev.h"

#include "private.h"

struct colormaps_prv {
  struct doom_prv   *drvdata;

  uint32_t          num;

  size_t            size;
  void              *colormaps;
  dma_addr_t        colormaps_dma;
};

long colormaps_create(struct doom_prv *drvdata, struct doomdev_ioctl_create_colormaps *args);
bool is_colormaps_fd(int fd);

#endif
