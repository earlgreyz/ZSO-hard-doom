#ifndef HARDDOOM_TEXTURE_H
#define HARDDOOM_TEXTURE_H

#include <linux/kernel.h>

#include "../include/doomdev.h"

#include "private.h"

struct texture_prv {
  struct doom_prv   *drvdata;

  uint16_t          height;

  size_t            size;
  void              *texture;
  struct pt_entry   *pt;

  dma_addr_t        texture_dma;
  dma_addr_t        pt_dma;
};

long texture_create(struct doom_prv *drvdata, struct doomdev_ioctl_create_texture *args);
bool is_texture_fd(int fd);

#endif
