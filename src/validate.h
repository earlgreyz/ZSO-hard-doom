#ifndef HARDDOOM_VALIDATE_H
#define HARDDOOM_VALIDATE_H

#include <linux/kernel.h>

#include "../include/doomdev.h"

#include "colormaps.h"
#include "private.h"
#include "surface.h"
#include "texture.h"

bool is_valid_copy_rect(struct surface_prv *dst, struct surface_prv *src, struct doomdev_copy_rect __user *rect);
bool is_valid_fill_rect(struct surface_prv *dst, struct doomdev_fill_rect __user *rect);
bool is_valid_draw_line(struct surface_prv *dst, struct doomdev_line __user *line);
bool is_valid_draw_column(struct surface_prv *dst, struct doomdev_column __user *column);
bool is_valid_draw_span(struct surface_prv *dst, struct doomdev_span __user *span);

#endif
