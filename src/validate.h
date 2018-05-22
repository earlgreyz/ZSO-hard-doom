#ifndef HARDDOOM_VALIDATE_H
#define HARDDOOM_VALIDATE_H

#include <linux/kernel.h>

#include "../include/doomdev.h"

#include "colormaps.h"
#include "private.h"
#include "surface.h"
#include "texture.h"

/**
 * Validates if a given rect is a proper rect for the dst and src surfaces.
 **/
bool is_valid_copy_rect(struct surface_prv *dst, struct surface_prv *src, struct doomdev_copy_rect *rect);

/**
 * Validates if a given rect is a proper rect for the dst surface.
 **/
bool is_valid_fill_rect(struct surface_prv *dst, struct doomdev_fill_rect *rect);

/**
 * Validates if a given line is a proper line for the dst surface.
 **/
bool is_valid_draw_line(struct surface_prv *dst, struct doomdev_line *line);

/**
 * Validates if a given columns is a proper column for the dst surface.
 **/
bool is_valid_draw_column(struct surface_prv *dst, struct doomdev_column *column);

/**
 * Validates if a given span is a proper span for the dst surface.
 **/
bool is_valid_draw_span(struct surface_prv *dst, struct doomdev_span *span);

#endif
