#ifndef HARDDOOM_SELECT_H
#define HARDDOOM_SELECT_H

#include <linux/kernel.h>

#include "colormaps.h"
#include "private.h"
#include "surface.h"
#include "texture.h"

#define SELECT_SURF_SRC    0x01
#define SELECT_SURF_DST    0x02
#define SELECT_SURF_DIMS   0x04

int select_surface(struct surface_prv *surface, int flags);

int select_texture(struct texture_prv *texture);

#define SELECT_CMAP_COLOR  0x01
#define SELECT_CMAP_TRANS  0x02

int select_colormap(struct colormaps_prv *colormaps, uint8_t idx, int flags);

#endif
