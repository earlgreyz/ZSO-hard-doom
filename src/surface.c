#include <linux/anon_inodes.h>
#include <linux/fs.h>
#include <linux/ioctl.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/vmalloc.h>

#include "../include/harddoom.h"

#include "cmd.h"
#include "colormaps.h"
#include "flat.h"
#include "pt.h"
#include "select.h"
#include "surface.h"
#include "texture.h"

#define SURFACE_FILE_TYPE "surface"

#define SURFACE_MAX_SIZE    2048
#define SURFACE_MIN_WIDTH     64
#define SURFACE_MIN_HEIGHT     1
#define SURFACE_WIDTH_MASK  0x7f

#define SELECT_DIMS      (1 << 0)

static int get_colormaps(struct doom_prv *drvdata, int fd, struct colormaps_prv **res) {
  struct fd colormaps_fd;
  struct colormaps_prv *colormaps;

  colormaps_fd = fdget(fd);
  if (!is_colormaps_fd(&colormaps_fd))
    return -EINVAL;

  colormaps = (struct colormaps_prv *) colormaps_fd.file->private_data;
  if (colormaps->drvdata != drvdata)
    return -EINVAL;

  *res = colormaps;
  return 0;
}

static int get_flat(struct doom_prv *drvdata, int fd, struct flat_prv **res) {
  struct fd flat_fd;
  struct flat_prv *flat;

  flat_fd = fdget(fd);
  if (!is_flat_fd(&flat_fd))
    return -EINVAL;

  flat = (struct flat_prv *) flat_fd.file->private_data;
  if (flat->drvdata != drvdata)
    return -EINVAL;

  *res = flat;
  return 0;
}

static int get_surface(struct doom_prv *drvdata, int fd, struct surface_prv **res) {
  struct fd surface_fd;
  struct surface_prv *surface;

  surface_fd = fdget(fd);
  if (!is_surface_fd(&surface_fd))
    return -EINVAL;

  surface = (struct surface_prv *) surface_fd.file->private_data;
  if (surface->drvdata != drvdata)
    return -EINVAL;

  *res = surface;
  return 0;
}

static int get_texture(struct doom_prv *drvdata, int fd, struct texture_prv **res) {
  struct fd texture_fd;
  struct texture_prv *texture;

  texture_fd = fdget(fd);
  if (!is_texture_fd(&texture_fd))
    return -EINVAL;

  texture = (struct texture_prv *) texture_fd.file->private_data;
  if (texture->drvdata != drvdata)
    return -EINVAL;

  *res = texture;
  return 0;
}

static long surface_copy_rects(struct file *file, struct doomdev_surf_ioctl_copy_rects *args) {
  long err;

  long i;
  struct doomdev_copy_rect *rect;
  struct surface_prv *prv = (struct surface_prv *) file->private_data;

  struct surface_prv *src_prv;

  if ((err = get_surface(prv->drvdata, args->surf_src_fd, &src_prv)))
    return err;

  mutex_lock(&prv->drvdata->cmd_mutex);
  if ((err = select_surface(prv, SELECT_SURF_DST | SELECT_SURF_DIMS)))
    goto fill_rects_surf_err;
  if ((err = select_surface(src_prv, SELECT_SURF_SRC)))
    goto fill_rects_surf_err;

  for (i = 0; i < args->rects_num; ++i) {
    rect = (struct doomdev_copy_rect *) args->rects_ptr + i;
    if ((err = doom_cmd(prv->drvdata, HARDDOOM_CMD_XY_A(rect->pos_dst_x, rect->pos_dst_y))))
      goto copy_rects_rect_err;
    if ((err = doom_cmd(prv->drvdata, HARDDOOM_CMD_XY_B(rect->pos_src_x, rect->pos_src_y))))
      goto copy_rects_rect_err;
    if ((err = doom_cmd(prv->drvdata, HARDDOOM_CMD_COPY_RECT(rect->width, rect->height))))
      goto copy_rects_rect_err;
  }
  mutex_unlock(&prv->drvdata->cmd_mutex);
  return i;

copy_rects_rect_err:
  err = i == 0? -EFAULT: i;
fill_rects_surf_err:
  mutex_unlock(&prv->drvdata->cmd_mutex);
  return err;
}

static long surface_fill_rects(struct file *file, struct doomdev_surf_ioctl_fill_rects *args) {
  unsigned long err;

  long i;
  struct doomdev_fill_rect *rect;
  struct surface_prv *prv = (struct surface_prv *) file->private_data;

  mutex_lock(&prv->drvdata->cmd_mutex);
  if ((err = select_surface(prv, SELECT_SURF_DST | SELECT_SURF_DIMS)))
    goto fill_rects_surf_err;

  for (i = 0; i < args->rects_num; ++i) {
    rect = (struct doomdev_fill_rect *) args->rects_ptr + i;
    if ((err = doom_cmd(prv->drvdata, HARDDOOM_CMD_FILL_COLOR(rect->color))))
      goto fill_rects_rect_err;
    if ((err = doom_cmd(prv->drvdata, HARDDOOM_CMD_XY_A(rect->pos_dst_x, rect->pos_dst_y))))
      goto fill_rects_rect_err;
    if ((err = doom_cmd(prv->drvdata, HARDDOOM_CMD_FILL_RECT(rect->width, rect->height))))
      goto fill_rects_rect_err;
  }
  mutex_unlock(&prv->drvdata->cmd_mutex);
  return i;

fill_rects_rect_err:
  err = i == 0? -EFAULT: i;
fill_rects_surf_err:
  mutex_unlock(&prv->drvdata->cmd_mutex);
  return err;
}

static long surface_draw_lines(struct file *file, struct doomdev_surf_ioctl_draw_lines *args) {
  unsigned long err;

  long i;
  struct doomdev_line *line;
  struct surface_prv *prv = (struct surface_prv *) file->private_data;

  mutex_lock(&prv->drvdata->cmd_mutex);
  if ((err = select_surface(prv, SELECT_SURF_DST | SELECT_SURF_DIMS)))
    goto fill_lines_dst_err;

  for (i = 0; i < args->lines_num; ++i) {
    line = (struct doomdev_line *) args->lines_ptr + i;
    if ((err = doom_cmd(prv->drvdata, HARDDOOM_CMD_FILL_COLOR(line->color))))
      goto fill_lines_line_err;
    if ((err = doom_cmd(prv->drvdata, HARDDOOM_CMD_XY_A(line->pos_a_x, line->pos_a_y))))
      goto fill_lines_line_err;
    if ((err = doom_cmd(prv->drvdata, HARDDOOM_CMD_XY_B(line->pos_b_x, line->pos_b_y))))
      goto fill_lines_line_err;
    if ((err = doom_cmd(prv->drvdata, HARDDOOM_CMD_DRAW_LINE)))
      goto fill_lines_line_err;
  }
  mutex_unlock(&prv->drvdata->cmd_mutex);
  return i;

fill_lines_line_err:
  err = i == 0? -EFAULT: i;
fill_lines_dst_err:
  mutex_unlock(&prv->drvdata->cmd_mutex);
  return err;
}

static long surface_draw_background(struct file *file, struct doomdev_surf_ioctl_draw_background *args) {
  long err;

  struct surface_prv *prv = (struct surface_prv *) file->private_data;
  struct flat_prv *flat_prv;

  if ((err = get_flat(prv->drvdata, args->flat_fd, &flat_prv)))
    return err;

  mutex_lock(&prv->drvdata->cmd_mutex);
  if ((err = select_surface(prv, SELECT_SURF_DST | SELECT_SURF_DIMS)))
    goto draw_background_err;
  if ((err = doom_cmd(prv->drvdata, HARDDOOM_CMD_FLAT_ADDR(flat_prv->flat_dma))))
    goto draw_background_err;
  if ((err = doom_cmd(prv->drvdata, HARDDOOM_CMD_DRAW_BACKGROUND)))
    goto draw_background_err;
  mutex_unlock(&prv->drvdata->cmd_mutex);
  return 0;

draw_background_err:
  mutex_unlock(&prv->drvdata->cmd_mutex);
  return err;
}

static long surface_draw_columns(struct file *file, struct doomdev_surf_ioctl_draw_columns *args) {
  unsigned long err;

  long i;
  struct doomdev_column *column;
  struct surface_prv *prv = (struct surface_prv *) file->private_data;

  bool use_texture, use_translations, use_colormaps;
  struct texture_prv *texture = NULL;
  struct colormaps_prv *colormaps = NULL, *translations = NULL;

  // HARDDOOM_DRAW_PARAMS_FUZZ ignores other flags
  if (args->draw_flags & HARDDOOM_DRAW_PARAMS_FUZZ) {
    args->draw_flags = HARDDOOM_DRAW_PARAMS_FUZZ;
  }

  use_texture = !(args->draw_flags & DOOMDEV_DRAW_FLAGS_FUZZ);
  use_translations = (args->draw_flags & DOOMDEV_DRAW_FLAGS_TRANSLATE);
  use_colormaps = (args->draw_flags & (DOOMDEV_DRAW_FLAGS_FUZZ | DOOMDEV_DRAW_FLAGS_COLORMAP));

  if (use_texture)
    if ((err = get_texture(prv->drvdata, args->texture_fd, &texture)))
      return err;

  if (use_translations)
    if ((err = get_colormaps(prv->drvdata, args->translations_fd, &translations)))
      return err;

  if (use_colormaps)
    if ((err = get_colormaps(prv->drvdata, args->colormaps_fd, &colormaps)))
      return err;

  mutex_lock(&prv->drvdata->cmd_mutex);
  if ((err = select_surface(prv, SELECT_SURF_DST | SELECT_SURF_DIMS)))
    goto draw_columns_err;
  if ((err = doom_cmd(prv->drvdata, HARDDOOM_CMD_DRAW_PARAMS(args->draw_flags))))
    goto draw_columns_err;

  if (use_texture)
    if ((err = select_texture(texture)))
      goto draw_columns_err;

  if (use_translations)
    if ((err = select_colormap(translations, args->translation_idx, SELECT_CMAP_TRANS)))
      goto draw_columns_err;

  for (i = 0; i < args->columns_num; ++i) {
    column = (struct doomdev_column *) args->columns_ptr + i;
    if ((err = doom_cmd(prv->drvdata, HARDDOOM_CMD_XY_A(column->x, column->y1))))
      goto draw_columns_column_err;
    if ((err = doom_cmd(prv->drvdata, HARDDOOM_CMD_XY_B(column->x, column->y2))))
      goto draw_columns_column_err;

    if (use_texture) {
      if ((err = doom_cmd(prv->drvdata, HARDDOOM_CMD_USTART(column->ustart))))
        goto draw_columns_column_err;
      if ((err = doom_cmd(prv->drvdata, HARDDOOM_CMD_USTEP(column->ustep))))
        goto draw_columns_column_err;
    }

    if (use_colormaps)
      if ((err = select_colormap(colormaps, column->colormap_idx, SELECT_CMAP_COLOR)))
        goto draw_columns_column_err;

    if ((err = doom_cmd(prv->drvdata, HARDDOOM_CMD_DRAW_COLUMN(column->texture_offset))))
      goto draw_columns_column_err;
  }
  mutex_unlock(&prv->drvdata->cmd_mutex);
  return i;

draw_columns_column_err:
  err = i == 0? -EFAULT: i;
draw_columns_err:
  mutex_unlock(&prv->drvdata->cmd_mutex);
  return err;
}

static long surface_draw_spans(struct file *file, struct doomdev_surf_ioctl_draw_spans *args) {
  unsigned long err;

  long i;
  struct doomdev_span *span;
  struct surface_prv *prv = (struct surface_prv *) file->private_data;

  bool use_translations, use_colormaps;
  struct flat_prv *flat = NULL;
  struct colormaps_prv *colormaps = NULL, *translations = NULL;

  use_translations = args->draw_flags & DOOMDEV_DRAW_FLAGS_TRANSLATE;
  use_colormaps = args->draw_flags & DOOMDEV_DRAW_FLAGS_COLORMAP;

  if ((err = get_flat(prv->drvdata, args->flat_fd, &flat)))
    return err;

  if (use_translations)
    if ((err = get_colormaps(prv->drvdata, args->translations_fd, &translations)))
      return err;

  if (use_colormaps)
    if ((err = get_colormaps(prv->drvdata, args->colormaps_fd, &colormaps)))
      return err;

  mutex_lock(&prv->drvdata->cmd_mutex);
  if ((err = select_surface(prv, SELECT_SURF_DST | SELECT_SURF_DIMS)))
    goto draw_spans_err;
  if ((err = doom_cmd(prv->drvdata, HARDDOOM_CMD_DRAW_PARAMS(args->draw_flags))))
    goto draw_spans_err;
  if ((err = doom_cmd(prv->drvdata, HARDDOOM_CMD_FLAT_ADDR(flat->flat_dma))))
    goto draw_spans_err;

  if (use_translations)
    if ((err = select_colormap(translations, args->translation_idx, SELECT_CMAP_TRANS)))
      goto draw_spans_err;

  for (i = 0; i < args->spans_num; ++i) {
    span = (struct doomdev_span *) args->spans_ptr + i;
    if ((err = doom_cmd(prv->drvdata, HARDDOOM_CMD_USTART(span->ustart))))
      goto draw_spans_span_err;
    if ((err = doom_cmd(prv->drvdata, HARDDOOM_CMD_USTEP(span->ustep))))
      goto draw_spans_span_err;
    if ((err = doom_cmd(prv->drvdata, HARDDOOM_CMD_VSTART(span->vstart))))
      goto draw_spans_span_err;
    if ((err = doom_cmd(prv->drvdata, HARDDOOM_CMD_VSTEP(span->vstep))))
      goto draw_spans_span_err;
    if ((err = doom_cmd(prv->drvdata, HARDDOOM_CMD_XY_A(span->x1, span->y))))
      goto draw_spans_span_err;
    if ((err = doom_cmd(prv->drvdata, HARDDOOM_CMD_XY_B(span->x2, span->y))))
      goto draw_spans_span_err;

    if (use_colormaps)
      if ((err = select_colormap(colormaps, span->colormap_idx, SELECT_CMAP_COLOR)))
        goto draw_spans_span_err;

    if ((err = doom_cmd(prv->drvdata, HARDDOOM_CMD_DRAW_SPAN)))
      goto draw_spans_span_err;
  }
  mutex_unlock(&prv->drvdata->cmd_mutex);
  return i;

draw_spans_span_err:
  err = i == 0? -EFAULT: i;
draw_spans_err:
  mutex_unlock(&prv->drvdata->cmd_mutex);
  return err;
}

static long surface_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
  switch (cmd) {
    case DOOMDEV_SURF_IOCTL_COPY_RECTS:
      return surface_copy_rects(file, (struct doomdev_surf_ioctl_copy_rects *) arg);
    case DOOMDEV_SURF_IOCTL_FILL_RECTS:
      return surface_fill_rects(file, (struct doomdev_surf_ioctl_fill_rects *) arg);
    case DOOMDEV_SURF_IOCTL_DRAW_LINES:
      return surface_draw_lines(file, (struct doomdev_surf_ioctl_draw_lines *) arg);
    case DOOMDEV_SURF_IOCTL_DRAW_BACKGROUND:
      return surface_draw_background(file, (struct doomdev_surf_ioctl_draw_background *) arg);
    case DOOMDEV_SURF_IOCTL_DRAW_COLUMNS:
      return surface_draw_columns(file, (struct doomdev_surf_ioctl_draw_columns *) arg);
    case DOOMDEV_SURF_IOCTL_DRAW_SPANS:
      return surface_draw_spans(file, (struct doomdev_surf_ioctl_draw_spans *) arg);
    default:
      return -ENOTTY;
  }
}

static loff_t surface_llseek(struct file *file, loff_t filepos, int whence) {
  loff_t pos = 0;

  struct surface_prv *prv = (struct surface_prv *) file->private_data;
  size_t size = prv->width * prv->height;

  switch (whence) {
    case SEEK_SET:
      pos = filepos;
      break;
    case SEEK_CUR:
      pos = file->f_pos + filepos;
      break;
    case SEEK_END:
      pos = size - filepos;
      break;
    default:
      return -EINVAL;
  }

  file->f_pos = pos;
  return pos;
}

static ssize_t surface_read(struct file *file, char __user *buf, size_t count, loff_t *filepos) {
  unsigned long err;

  struct surface_prv *prv = (struct surface_prv *) file->private_data;
  size_t copied, size = prv->width * prv->height;

  if (*filepos > size || *filepos < 0)
    return 0;

  // First queue to see if noone is pinging atm
  down(&prv->drvdata->ping_queue);
  // Send the ping and wait for the interrupt to wake us up
  if ((err = doom_cmd(prv->drvdata, HARDDOOM_CMD_PING_SYNC)))
    return -EFAULT;
  down(&prv->drvdata->ping_wait);

  if (count > size - *filepos)
    count = size - *filepos;

  err = copy_to_user(buf, prv->surface + *filepos, count);
  copied = count - err;
  if (copied == 0)
    return -EFAULT;

  *filepos = *filepos + copied;
  return copied;
}

static int surface_release(struct inode *ino, struct file *file) {
  struct surface_prv *prv = (struct surface_prv *) file->private_data;
  dma_free_coherent(prv->drvdata->pci, prv->size, prv->surface, prv->surface_dma);
  kfree(prv);
  return 0;
}

static struct file_operations surface_ops = {
  .owner = THIS_MODULE,
  .unlocked_ioctl = surface_ioctl,
	.compat_ioctl = surface_ioctl,
  .llseek = surface_llseek,
  .read = surface_read,
  .release = surface_release,
};

bool is_surface_fd(struct fd *fd) {
  return (fd->file != NULL) && (fd->file->f_op == &surface_ops);
}

static int allocate_surface(struct surface_prv *prv, size_t size) {
  long pt_len;
  size_t pt_size;
  size_t aligned_size = ALIGN(size, PT_ALIGNMENT);

  if ((pt_len = pt_length(size)) <= 0) {
    return pt_len;
  }

  pt_size = pt_len * sizeof(struct pt_entry);
  prv->size = aligned_size + pt_size;

  prv->surface = dma_alloc_coherent(prv->drvdata->pci, prv->size, &prv->surface_dma, GFP_KERNEL);
  if (prv->surface == NULL) {
    return -ENOMEM;
  }

  prv->pt = (struct pt_entry *) (prv->surface + aligned_size);
  prv->pt_dma = prv->surface_dma + aligned_size;
  pt_fill(prv->surface_dma, prv->pt, pt_len);
  return 0;
}

long surface_create(struct doom_prv *drvdata, struct doomdev_ioctl_create_surface *args) {
  long err;

  struct surface_prv *prv;
  int fd;
  struct fd surface_fd;

  if (args->width > SURFACE_MAX_SIZE || args->height > SURFACE_MAX_SIZE) {
    return -EOVERFLOW;
  } else if (args->width < SURFACE_MIN_WIDTH
      || (args->width & SURFACE_WIDTH_MASK) != 0
      || args->height < SURFACE_MIN_HEIGHT) {
    return -EINVAL;
  }

  prv = (struct surface_prv *) kmalloc(sizeof(struct surface_prv), GFP_KERNEL);
  if (prv == NULL) {
    printk(KERN_WARNING "[doom_surface] surface_create error: kmalloc\n");
    err = -ENOMEM;
    goto create_kmalloc_err;
  }

  *prv = (struct surface_prv){
    .drvdata = drvdata,
    .width = args->width,
    .height = args->height,
  };

  if ((err = allocate_surface(prv, args->width * args->height))) {
    printk(KERN_WARNING "[doom_surface] surface_create error: allocate_surface\n");
    goto create_allocate_err;
  }

  fd = anon_inode_getfd(SURFACE_FILE_TYPE, &surface_ops, prv, O_RDWR);
  if (fd < 0) {
    printk(KERN_WARNING "[doom_surface] surface_create error: anon_inode_getfd\n");
    err = fd;
    goto create_getfd_err;
  }

  surface_fd = fdget(fd);
  surface_fd.file->f_mode |= FMODE_LSEEK | FMODE_PREAD | FMODE_PWRITE;
  return fd;

create_getfd_err:
  dma_free_coherent(prv->drvdata->pci, prv->size, prv->surface, prv->surface_dma);
create_allocate_err:
  kfree(prv);
create_kmalloc_err:
  return err;
}
