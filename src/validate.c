#include "validate.h"

static bool is_on_surface(struct surface_prv *surface, uint16_t x, uint16_t y) {
  return (x <= surface->width) && (y <= surface->height);
}

bool is_valid_copy_rect(struct surface_prv *dst, struct surface_prv *src, struct doomdev_copy_rect __user *rect) {
  return (is_on_surface(src, rect->pos_src_x, rect->pos_src_y)
      && is_on_surface(src, rect->pos_src_x + rect->width, rect->pos_src_y + rect->height)
      && is_on_surface(dst, rect->pos_dst_x, rect->pos_dst_y)
      && is_on_surface(dst, rect->pos_dst_x + rect->width, rect->pos_dst_y + rect->height));
}

bool is_valid_fill_rect(struct surface_prv *dst, struct doomdev_fill_rect __user *rect) {
  return (is_on_surface(dst, rect->pos_dst_x, rect->pos_dst_y)
      && is_on_surface(dst, rect->pos_dst_x + rect->width, rect->pos_dst_y + rect->height));
}

bool is_valid_draw_line(struct surface_prv *dst, struct doomdev_line __user *line) {
  return (is_on_surface(dst, line->pos_a_x, line->pos_a_y)
      && is_on_surface(dst, line->pos_b_x, line->pos_b_y));
}

bool is_valid_draw_column(struct surface_prv *dst, struct doomdev_column __user *column) {
  return (is_on_surface(dst, column->x, column->y1)
      && is_on_surface(dst, column->x, column->y2));
}

bool is_valid_draw_span(struct surface_prv *dst, struct doomdev_span __user *span) {
  return (is_on_surface(dst, span->x1, span->y)
      && is_on_surface(dst, span->x2, span->y));
}
