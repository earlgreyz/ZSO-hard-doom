#ifndef HARDDOOM_FLAT_H
#define HARDDOOM_FLAT_H

#include <linux/kernel.h>
#include <linux/file.h>

#include "../include/doomdev.h"

#include "private.h"

struct flat_prv {
  struct doom_prv   *drvdata;

  void              *flat;
  dma_addr_t        flat_dma;
};

long flat_create(struct doom_prv *drvdata, struct doomdev_ioctl_create_flat *args);
bool is_flat_fd(struct fd *fd);

#endif
