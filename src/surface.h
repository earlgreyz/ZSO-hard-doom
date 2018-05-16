#ifndef HARDDOOM_SURFACE_H
#define HARDDOOM_SURFACE_H

#include <linux/kernel.h>
#include <linux/file.h>

#include "../include/doomdev.h"

#include "private.h"

long surface_create(struct doom_prv *drvdata, struct doomdev_ioctl_create_surface *args);
bool is_surface_fd(struct fd *fd);

#endif
