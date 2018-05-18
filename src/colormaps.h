#ifndef HARDDOOM_COLORMAP_H
#define HARDDOOM_COLORMAP_H

#include <linux/kernel.h>
#include <linux/file.h>

#include "../include/doomdev.h"

#include "private.h"

/// Stored as a private_data of the colormaps descriptor.
struct colormaps_prv {
  struct doom_prv   *drvdata;       // Harddoom driver data

  uint32_t          num;            // Colormaps count
  size_t            size;           // Allocated memory size

  void              *colormaps;     // Colormaps allocated memory ptr
  dma_addr_t        colormaps_dma;  // DMA address of the colormaps
};

/**
 * Creates a new colormaps array.
 * @success returns a newly created colormaps file descriptor.
 * @failure returns a negated error code.
 **/
long colormaps_create(struct doom_prv *drvdata, struct doomdev_ioctl_create_colormaps *args);

/**
 * Checks if a given fd is a colormaps file descriptor.
 * false is also retuned when the fd doesn't correspond to any open file.
 **/
bool is_colormaps_fd(struct fd *fd);

/**
 * Calculates the DMA address of a colormap at the given index.
 * @success returns 0 and stores the address at @param addr.
 * @failure returns a negated error code.
 **/
int colormaps_get_addr(struct colormaps_prv *prv, uint8_t idx, dma_addr_t *addr);

#endif
