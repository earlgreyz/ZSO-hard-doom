#ifndef HARDDOOM_SURFACE_H
#define HARDDOOM_SURFACE_H

#include <linux/kernel.h>

#include "private.h"
#include "../include/doomdev.h"

long surface_create(struct doom_prv *drvdata, struct doomdev_ioctl_create_surface *args);

#endif
