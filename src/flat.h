#ifndef HARDDOOM_FLAT_H
#define HARDDOOM_FLAT_H

#include <linux/kernel.h>
#include <linux/file.h>

#include "../include/doomdev.h"

#include "private.h"

/// Stored as a private_data of the flat descriptor.
struct flat_prv {
  struct doom_prv   *drvdata;  // Harddoom driver data

  void              *flat;     // Allocated memory ptr
  dma_addr_t        flat_dma;  // DMA address of the flat
};

/**
 * Creates a new flat.
 * @success returns a newly created flat file descriptor.
 * @failure returns a negated error code.
 **/
long flat_create(struct doom_prv *drvdata, struct doomdev_ioctl_create_flat *args);

/**
 * Finds a flat created on the same device based on the fd.
 * @success stores a result in @param res and returns 0.
 * @failure returns a negated error code.
 **/
int flat_get(struct doom_prv *drvdata, int fd, struct flat_prv **res);

/**
 * Checks if a given fd is a flat file descriptor.
 * false is also retuned when the fd doesn't correspond to any open file.
 **/
bool is_flat_fd(struct fd *fd);

#endif
