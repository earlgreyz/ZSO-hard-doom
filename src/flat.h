#ifndef HARDDOOM_FLAT_H
#define HARDDOOM_FLAT_H

#include <linux/kernel.h>

#include "private.h"
#include "../include/doomdev.h"

long flat_create(struct doom_prv *drvdata, struct doomdev_ioctl_create_flat *args);

#endif
