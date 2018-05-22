#include "../include/harddoom.h"

#include "cmd.h"
#include "private.h"
#include "select.h"

#undef _MUST
#define _MUST(s) if ((err = (s))) return err

static int select_surface_src(struct surface_prv *surface) {
  int err;

  if (surface->drvdata->surf_src == surface->surface_dma)
    return 0;

  _MUST(cmd(surface->drvdata, HARDDOOM_CMD_SURF_SRC_PT(surface->pt_dma)));
  surface->drvdata->surf_src = surface->surface_dma;

  return 0;
}

static int select_surface_dst(struct surface_prv *surface) {
  int err;

  surface->dirty = true;

  if (surface->drvdata->surf_dst == surface->surface_dma)
    return 0;

  _MUST(cmd(surface->drvdata, HARDDOOM_CMD_SURF_DST_PT(surface->pt_dma)));
  surface->drvdata->surf_dst = surface->surface_dma;

  return 0;
}

static int select_surface_dims(struct doom_prv *drvdata, uint32_t width, uint32_t height) {
  int err;

  if (drvdata->surf_width == width && drvdata->surf_height == height)
    return 0;

  _MUST(cmd(drvdata, HARDDOOM_CMD_SURF_DIMS(width, height)));
  drvdata->surf_width = width;
  drvdata->surf_height = height;

  return 0;
}

int select_surface(struct surface_prv *surface, int flags) {
  int err;

  if (flags & SELECT_SURF_SRC & SELECT_SURF_DST)
    return -EINVAL;

  if (flags & SELECT_SURF_SRC)
    _MUST(select_surface_src(surface));

  if (flags & SELECT_SURF_DST)
    _MUST(select_surface_dst(surface));

  if (flags & SELECT_SURF_DIMS)
    _MUST(select_surface_dims(surface->drvdata, surface->width, surface->height));

  return 0;
}

int select_texture(struct texture_prv *texture) {
  int err;

  if (texture->drvdata->texture == texture->texture_dma)
    return 0;

  _MUST(cmd(texture->drvdata, HARDDOOM_CMD_TEXTURE_PT(texture->pt_dma)));
  _MUST(cmd(texture->drvdata, HARDDOOM_CMD_TEXTURE_DIMS(texture->size, texture->height)));
  texture->drvdata->texture = texture->texture_dma;
  return 0;
}

int select_colormap(struct colormaps_prv *colormaps, uint8_t idx, int flags) {
  int err;
  dma_addr_t addr;

  if (flags & SELECT_CMAP_COLOR & SELECT_CMAP_TRANS)
    return -EINVAL;

  _MUST(colormaps_at(colormaps, idx, &addr));

  if (flags & SELECT_CMAP_COLOR) {
    if (colormaps->drvdata->colormap == addr)
      return 0;

    _MUST(cmd(colormaps->drvdata, HARDDOOM_CMD_COLORMAP_ADDR(addr)));
    colormaps->drvdata->colormap = addr;
  }

  if (flags & SELECT_CMAP_TRANS) {
    if (colormaps->drvdata->translation == addr)
      return 0;

    _MUST(cmd(colormaps->drvdata, HARDDOOM_CMD_TRANSLATION_ADDR(addr)));
    colormaps->drvdata->translation = addr;
  }

  return 0;
}

#undef _MUST
