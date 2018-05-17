#ifndef HARDDOOM_TEXTURE_H
#define HARDDOOM_TEXTURE_H

#include <linux/kernel.h>
#include <linux/file.h>

#include "../include/doomdev.h"

#include "private.h"

/// Stored as a private_data of the texture descriptor.
struct texture_prv {
  struct doom_prv   *drvdata;    // Harddoom driver data

  uint16_t          height;      // Texture height in texels
  uint32_t          size_m1;     // Texture size in 256 B blocks -1

  size_t            size;        // Allocated memory size
  void              *texture;    // Allocated texture ptr
  struct pt_entry   *pt;         // Allocated page table ptr

  dma_addr_t        texture_dma; // DMA address of the texture
  dma_addr_t        pt_dma;      // DMA address of the page table
};

/**
 * Creates a new texture.
 * @success returns a newly created texture file descriptor.
 * @failure returns a negated error code.
 **/
long texture_create(struct doom_prv *drvdata, struct doomdev_ioctl_create_texture *args);

/**
 * Checks if a given fd is a texture file descriptor.
 * false is also retuned when the fd doesn't correspond to any open file.
 **/
bool is_texture_fd(struct fd *fd);

#endif
