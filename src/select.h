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

/**
 * Selects surface if it differs from the currently selected.
 * Before calling select one should make sure that the device queue has
 * at least 2 free spots.
 *
 * SELECT_SURF_SRC change the SURF_SRC address.
 * SELECT_SURF_DST change the SURF_DST address.
 * SELECT_SURF_DIMS change the SURF_DIMS.
 *
 * @success returns 0.
 * @failure returns negated error code.
 **/
int __must_check select_surface(struct surface_prv *surface, int flags);

/**
 * Selects texture if it differs from the currently selected.
 * Before calling select one should make sure that the device queue has
 * at least 2 free spots.
 * @success returns 0.
 * @failure returns negated error code.
 **/
int __must_check select_texture(struct texture_prv *texture);

#define SELECT_CMAP_COLOR  0x01
#define SELECT_CMAP_TRANS  0x02

/**
 * Selects colormap if it differs from the currently selected.
 * Before calling select one should make sure that the device queue has
 * at least 2 free spots.
 *
 * SELECT_CMAP_COLOR change the COLORMAP address.
 * SELECT_CMAP_TRANS change the TRANSLATION address.
 *
 * @success returns 0.
 * @failure returns negated error code.
 **/
int __must_check select_colormap(struct colormaps_prv *colormaps, uint8_t idx, int flags);

#endif
