#ifndef HARDDOOM_SURFACE_H
#define HARDDOOM_SURFACE_H

#include <linux/kernel.h>

#include "private.h"

long doomsurf_create(struct doom_prv *drvdata, uint16_t width, uint16_t height);

#endif
