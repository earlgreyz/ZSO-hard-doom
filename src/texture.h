#ifndef HARDDOOM_TEXTURE_H
#define HARDDOOM_TEXTURE_H

#include <linux/kernel.h>

#include "private.h"
#include "../include/doomdev.h"

long texture_create(struct doom_prv *drvdata, struct doomdev_ioctl_create_texture *args);

#endif
