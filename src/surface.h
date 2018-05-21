#ifndef HARDDOOM_SURFACE_H
#define HARDDOOM_SURFACE_H

#include <linux/kernel.h>
#include <linux/file.h>

#include "../include/doomdev.h"

#include "private.h"

/// Stored as a private_data of the surface descriptor.
struct surface_prv {
  struct doom_prv   *drvdata;     // Harddoom driver data

  uint32_t          width;        // Surface width in pixels
  uint32_t          height;       // Surface height in pixels

  size_t            size;         // Allocated memory size
  void              *surface;     // Allocated surface ptr
  struct pt_entry   *pt;          // Allocated page table ptr

  dma_addr_t        surface_dma;  // DMA address of the surface
  dma_addr_t        pt_dma;       // DMA address of the page table

  bool              dirty;        // If the buffer has recently been drawn on
};

/**
 * Creates a new surface.
 * @success returns a newly created surface file descriptor.
 * @failure returns a negated error code.
 **/
long surface_create(struct doom_prv *drvdata, struct doomdev_ioctl_create_surface *args);

/**
 * Checks if a given fd is a surface file descriptor.
 * false is also retuned when the fd doesn't correspond to any open file.
 **/
bool is_surface_fd(struct fd *fd);

/**
 * Finds a surface created on the same device based on the fd.
 * @success stores a result in @param res and returns 0.
 * @failure returns a negated error code.
 **/
int surface_get(struct doom_prv *drvdata, struct fd *fd, struct surface_prv **res);

#endif
