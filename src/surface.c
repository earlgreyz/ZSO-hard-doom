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
#include "validate.h"

#define SURFACE_FILE_TYPE "surface"

#define SURFACE_MAX_SIZE    2048
#define SURFACE_MIN_WIDTH     64
#define SURFACE_MIN_HEIGHT     1
#define SURFACE_WIDTH_MASK  0x7f

#undef _MUST
#define _MUST(s) if ((err = (s))) goto cmd_err

static int __must_check fence_next(struct doom_prv *drvdata) {
  int err;
  unsigned long flags;

  spin_lock_irqsave(&drvdata->fence_lock, flags);
  drvdata->fence = (drvdata->fence + 1) % HARDDOOM_FENCE_MASK;
  _MUST(cmd(drvdata, HARDDOOM_CMD_FENCE(drvdata->fence)));
  spin_unlock_irqrestore(&drvdata->fence_lock, flags);

  cmd_commit(drvdata);
  return 0;

cmd_err:
  spin_unlock_irqrestore(&drvdata->fence_lock, flags);
  return err;
}

static long surface_copy_rects(struct file *file, struct doomdev_surf_ioctl_copy_rects __user *uargs) {
  long err;

  long i = 0;
  struct surface_prv *prv = (struct surface_prv *) file->private_data;

  struct fd src_fd;
  struct surface_prv *src_prv;

  struct doomdev_surf_ioctl_copy_rects args;
  struct doomdev_copy_rect rect;

  if (copy_from_user(&args, uargs, sizeof(args)))
    return -EFAULT;

  src_fd = fdget(args.surf_src_fd);
  if ((err = surface_get(prv->drvdata, &src_fd, &src_prv)))
    goto src_err;

  if ((err = mutex_lock_interruptible(&prv->drvdata->cmd_mutex)))
    goto mutex_err;

  _MUST(cmd_wait(prv->drvdata, 4));
  _MUST(select_surface(prv, SELECT_SURF_DST | SELECT_SURF_DIMS));
  _MUST(select_surface(src_prv, SELECT_SURF_SRC));
  if (src_prv->dirty) {
    _MUST(cmd(prv->drvdata, HARDDOOM_CMD_INTERLOCK));
    src_prv->dirty = false;
  }
  cmd_commit(prv->drvdata);

  for (i = 0; i < args.rects_num; ++i) {
    if (copy_from_user(&rect, (void __user *) args.rects_ptr + i * sizeof(rect), sizeof(rect))) {
      err = -EFAULT;
      goto cmd_err;
    }

    if (!is_valid_copy_rect(prv, src_prv, &rect)) {
      err = -EINVAL;
      goto cmd_err;
    }

    _MUST(cmd_wait(prv->drvdata, 4));
    _MUST(cmd(prv->drvdata, HARDDOOM_CMD_XY_A(rect.pos_dst_x, rect.pos_dst_y)));
    _MUST(cmd(prv->drvdata, HARDDOOM_CMD_XY_B(rect.pos_src_x, rect.pos_src_y)));
    _MUST(cmd(prv->drvdata, HARDDOOM_CMD_COPY_RECT(rect.width, rect.height)));
    cmd_commit(prv->drvdata);
  }

  _MUST(fence_next(prv->drvdata));
  mutex_unlock(&prv->drvdata->cmd_mutex);

  fdput(src_fd);
  return i;

cmd_err:
  err = i == 0 ? err : i;
  if (fence_next(prv->drvdata))
    printk(KERN_WARNING "FENCE_NEXT failed when handling an error");
  mutex_unlock(&prv->drvdata->cmd_mutex);
mutex_err:
src_err:
  fdput(src_fd);
  return err;
}

static long surface_fill_rects(struct file *file, struct doomdev_surf_ioctl_fill_rects __user *uargs) {
  long err;

  long i = 0;
  struct surface_prv *prv = (struct surface_prv *) file->private_data;

  struct doomdev_surf_ioctl_fill_rects args;
  struct doomdev_fill_rect rect;

  if (copy_from_user(&args, (void __user *) uargs, sizeof(args)))
    return -EFAULT;

  if ((err = mutex_lock_interruptible(&prv->drvdata->cmd_mutex)))
    return err;

  _MUST(cmd_wait(prv->drvdata, 2));
  _MUST(select_surface(prv, SELECT_SURF_DST | SELECT_SURF_DIMS));
  cmd_commit(prv->drvdata);

  for (i = 0; i < args.rects_num; ++i) {
    if (copy_from_user(&rect, (void __user *) args.rects_ptr + i * sizeof(rect), sizeof(rect))) {
      err = -EFAULT;
      goto cmd_err;
    }

    if (!is_valid_fill_rect(prv, &rect)) {
      err = -EINVAL;
      goto cmd_err;
    }

    _MUST(cmd_wait(prv->drvdata, 4));
    _MUST(cmd(prv->drvdata, HARDDOOM_CMD_FILL_COLOR(rect.color)));
    _MUST(cmd(prv->drvdata, HARDDOOM_CMD_XY_A(rect.pos_dst_x, rect.pos_dst_y)));
    _MUST(cmd(prv->drvdata, HARDDOOM_CMD_FILL_RECT(rect.width, rect.height)));
    cmd_commit(prv->drvdata);
  }

  _MUST(fence_next(prv->drvdata));
  mutex_unlock(&prv->drvdata->cmd_mutex);
  return i;

cmd_err:
  err = i == 0 ? err : i;
  if (fence_next(prv->drvdata))
    printk(KERN_WARNING "FENCE_NEXT failed when handling an error");
  mutex_unlock(&prv->drvdata->cmd_mutex);
  return err;
}

static long surface_draw_lines(struct file *file, struct doomdev_surf_ioctl_draw_lines __user *uargs) {
  long err;

  long i = 0;
  struct surface_prv *prv = (struct surface_prv *) file->private_data;

  struct doomdev_surf_ioctl_draw_lines args;
  struct doomdev_line line;

  if (copy_from_user(&args, uargs, sizeof(args)))
    return -EFAULT;

  if ((err = mutex_lock_interruptible(&prv->drvdata->cmd_mutex)))
    return err;

  _MUST(cmd_wait(prv->drvdata, 2));
  _MUST(select_surface(prv, SELECT_SURF_DST | SELECT_SURF_DIMS));
  cmd_commit(prv->drvdata);

  for (i = 0; i < args.lines_num; ++i) {
    if (copy_from_user(&line, (void __user *) args.lines_ptr + i * sizeof(line), sizeof(line))) {
      err = -EFAULT;
      goto cmd_err;
    }

    if (!is_valid_draw_line(prv, &line)) {
      err = -EINVAL;
      goto cmd_err;
    }

    _MUST(cmd_wait(prv->drvdata, 5));
    _MUST(cmd(prv->drvdata, HARDDOOM_CMD_FILL_COLOR(line.color)));
    _MUST(cmd(prv->drvdata, HARDDOOM_CMD_XY_A(line.pos_a_x, line.pos_a_y)));
    _MUST(cmd(prv->drvdata, HARDDOOM_CMD_XY_B(line.pos_b_x, line.pos_b_y)));
    _MUST(cmd(prv->drvdata, HARDDOOM_CMD_DRAW_LINE));
    cmd_commit(prv->drvdata);
  }

  _MUST(fence_next(prv->drvdata));
  mutex_unlock(&prv->drvdata->cmd_mutex);
  return i;

cmd_err:
  err = i == 0 ? err : i;
  if (fence_next(prv->drvdata))
    printk(KERN_WARNING "FENCE_NEXT failed when handling an error");
  mutex_unlock(&prv->drvdata->cmd_mutex);
  return err;
}

static long surface_draw_background(struct file *file, struct doomdev_surf_ioctl_draw_background __user *uargs) {
  long err;

  struct surface_prv *prv = (struct surface_prv *) file->private_data;

  struct doomdev_surf_ioctl_draw_background args;

  struct fd flat_fd;
  struct flat_prv *flat_prv;

  if (copy_from_user(&args, uargs, sizeof(args)))
    return -EFAULT;

  flat_fd = fdget(args.flat_fd);
  if ((err = flat_get(prv->drvdata, &flat_fd, &flat_prv)))
    goto flat_err;

  if ((err = mutex_lock_interruptible(&prv->drvdata->cmd_mutex)))
    goto mutex_err;

  _MUST(cmd_wait(prv->drvdata, 5));
  _MUST(select_surface(prv, SELECT_SURF_DST | SELECT_SURF_DIMS));
  _MUST(cmd(prv->drvdata, HARDDOOM_CMD_FLAT_ADDR(flat_prv->flat_dma)));
  _MUST(cmd(prv->drvdata, HARDDOOM_CMD_DRAW_BACKGROUND));
  _MUST(fence_next(prv->drvdata));

  mutex_unlock(&prv->drvdata->cmd_mutex);

  fdput(flat_fd);
  return 0;

cmd_err:
  if (fence_next(prv->drvdata))
    printk(KERN_WARNING "FENCE_NEXT failed when handling error");
  mutex_unlock(&prv->drvdata->cmd_mutex);
mutex_err:
flat_err:
  fdput(flat_fd);
  return err;
}

static long surface_draw_columns(struct file *file, struct doomdev_surf_ioctl_draw_columns __user *uargs) {
  long err;

  long i = 0;
  struct surface_prv *prv = (struct surface_prv *) file->private_data;

  struct doomdev_surf_ioctl_draw_columns args;
  struct doomdev_column column;

  bool use_texture, use_translations, use_colormaps;
  struct fd texture_fd = {};
  struct texture_prv *texture = NULL;
  struct fd colormaps_fd = {}, translations_fd = {};
  struct colormaps_prv *colormaps = NULL, *translations = NULL;

  if (copy_from_user(&args, uargs, sizeof(args)))
    return -EFAULT;

  use_texture = !(args.draw_flags & DOOMDEV_DRAW_FLAGS_FUZZ);
  use_translations = (args.draw_flags & DOOMDEV_DRAW_FLAGS_TRANSLATE);
  use_colormaps = (args.draw_flags & (DOOMDEV_DRAW_FLAGS_FUZZ | DOOMDEV_DRAW_FLAGS_COLORMAP));

  if (use_texture) {
    texture_fd = fdget(args.texture_fd);
    if ((err = texture_get(prv->drvdata, &texture_fd, &texture)))
      goto texture_err;
  }

  if (use_translations) {
    translations_fd = fdget(args.translations_fd);
    if ((err = colormaps_get(prv->drvdata, &translations_fd, &translations)))
      goto translations_err;
  }

  if (use_colormaps) {
    colormaps_fd = fdget(args.colormaps_fd);
    if ((err = colormaps_get(prv->drvdata, &colormaps_fd, &colormaps)))
      goto colormaps_err;
  }

  if ((err = mutex_lock_interruptible(&prv->drvdata->cmd_mutex)))
    goto mutex_err;

  _MUST(cmd_wait(prv->drvdata, 6));
  _MUST(select_surface(prv, SELECT_SURF_DST | SELECT_SURF_DIMS));

  if (use_texture)
    _MUST(select_texture(texture));

  if (use_translations)
    _MUST(select_colormap(translations, args.translation_idx, SELECT_CMAP_TRANS));

  _MUST(cmd(prv->drvdata, HARDDOOM_CMD_DRAW_PARAMS(args.draw_flags)));
  cmd_commit(prv->drvdata);

  for (i = 0; i < args.columns_num; ++i) {
    if (copy_from_user(&column, (void __user *) args.columns_ptr + i * sizeof(column), sizeof(column))) {
      err = -EFAULT;
      goto cmd_err;
    }

    if (!is_valid_draw_column(prv, &column)) {
      err = -EINVAL;
      goto cmd_err;
    }

    _MUST(cmd_wait(prv->drvdata, 7));
    _MUST(cmd(prv->drvdata, HARDDOOM_CMD_XY_A(column.x, column.y1)));
    _MUST(cmd(prv->drvdata, HARDDOOM_CMD_XY_B(column.x, column.y2)));

    if (use_texture) {
      _MUST(cmd(prv->drvdata, HARDDOOM_CMD_USTART(column.ustart)));
      _MUST(cmd(prv->drvdata, HARDDOOM_CMD_USTEP(column.ustep)));
    }

    if (use_colormaps)
      _MUST(select_colormap(colormaps, column.colormap_idx, SELECT_CMAP_COLOR));

    _MUST(cmd(prv->drvdata, HARDDOOM_CMD_DRAW_COLUMN(column.texture_offset)));
    cmd_commit(prv->drvdata);
  }

  _MUST(fence_next(prv->drvdata));
  mutex_unlock(&prv->drvdata->cmd_mutex);

  if (use_colormaps)
    fdput(colormaps_fd);
  if (use_translations)
    fdput(translations_fd);
  if (use_texture)
    fdput(texture_fd);
  return i;

cmd_err:
  err = i == 0 ? err : i;
  if (fence_next(prv->drvdata))
    printk(KERN_WARNING "FENCE_NEXT failed when handling error");
  mutex_unlock(&prv->drvdata->cmd_mutex);
mutex_err:
colormaps_err:
  if (use_colormaps)
    fdput(colormaps_fd);
translations_err:
  if (use_translations)
    fdput(translations_fd);
texture_err:
  if (use_texture)
    fdput(texture_fd);
  return err;
}

static long surface_draw_spans(struct file *file, struct doomdev_surf_ioctl_draw_spans __user *args) {
  long err;

  long i = 0;
  struct doomdev_span __user *span;
  struct surface_prv *prv = (struct surface_prv *) file->private_data;

  bool use_translations, use_colormaps;
  struct fd flat_fd = {};
  struct flat_prv *flat = NULL;
  struct fd colormaps_fd = {}, translations_fd = {};
  struct colormaps_prv *colormaps = NULL, *translations = NULL;

  use_translations = args->draw_flags & DOOMDEV_DRAW_FLAGS_TRANSLATE;
  use_colormaps = args->draw_flags & DOOMDEV_DRAW_FLAGS_COLORMAP;

  flat_fd = fdget(args->flat_fd);
  if ((err = flat_get(prv->drvdata, &flat_fd, &flat)))
    goto flat_err;

  if (use_translations) {
    translations_fd = fdget(args->translations_fd);
    if ((err = colormaps_get(prv->drvdata, &translations_fd, &translations)))
      goto translations_err;
  }

  if (use_colormaps) {
    colormaps_fd = fdget(args->colormaps_fd);
    if ((err = colormaps_get(prv->drvdata, &colormaps_fd, &colormaps)))
      goto colormaps_err;
  }

  if ((err = mutex_lock_interruptible(&prv->drvdata->cmd_mutex)))
    goto mutex_err;

  _MUST(cmd_wait(prv->drvdata, 5));
  _MUST(select_surface(prv, SELECT_SURF_DST | SELECT_SURF_DIMS));
  _MUST(cmd(prv->drvdata, HARDDOOM_CMD_DRAW_PARAMS(args->draw_flags)));
  _MUST(cmd(prv->drvdata, HARDDOOM_CMD_FLAT_ADDR(flat->flat_dma)));

  if (use_translations)
    _MUST(select_colormap(translations, args->translation_idx, SELECT_CMAP_TRANS));

  cmd_commit(prv->drvdata);

  for (i = 0; i < args->spans_num; ++i) {
    span = (struct doomdev_span __user *) args->spans_ptr + i;
    if (!is_valid_draw_span(prv, span)) {
      err = -EINVAL;
      goto cmd_err;
    }

    _MUST(cmd_wait(prv->drvdata, 9));
    _MUST(cmd(prv->drvdata, HARDDOOM_CMD_USTART(span->ustart)));
    _MUST(cmd(prv->drvdata, HARDDOOM_CMD_USTEP(span->ustep)));
    _MUST(cmd(prv->drvdata, HARDDOOM_CMD_VSTART(span->vstart)));
    _MUST(cmd(prv->drvdata, HARDDOOM_CMD_VSTEP(span->vstep)));
    _MUST(cmd(prv->drvdata, HARDDOOM_CMD_XY_A(span->x1, span->y)));
    _MUST(cmd(prv->drvdata, HARDDOOM_CMD_XY_B(span->x2, span->y)));

    if (use_colormaps)
      _MUST(select_colormap(colormaps, span->colormap_idx, SELECT_CMAP_COLOR));

    _MUST(cmd(prv->drvdata, HARDDOOM_CMD_DRAW_SPAN));
    cmd_commit(prv->drvdata);
  }

  _MUST(fence_next(prv->drvdata));
  mutex_unlock(&prv->drvdata->cmd_mutex);

  if (use_colormaps)
    fdput(colormaps_fd);
  if (use_translations)
    fdput(translations_fd);
  fdput(flat_fd);
  return i;

cmd_err:
  err = i == 0 ? err : i;
  if (fence_next(prv->drvdata))
    printk(KERN_WARNING "FENCE_NEXT failed when handling error");
  mutex_unlock(&prv->drvdata->cmd_mutex);
mutex_err:
colormaps_err:
  if (use_colormaps)
    fdput(colormaps_fd);
translations_err:
  if (use_translations)
    fdput(translations_fd);
flat_err:
  fdput(flat_fd);
  return err;
}

static long surface_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
  switch (cmd) {
    case DOOMDEV_SURF_IOCTL_COPY_RECTS:
      return surface_copy_rects(file, (struct doomdev_surf_ioctl_copy_rects __user *) arg);
    case DOOMDEV_SURF_IOCTL_FILL_RECTS:
      return surface_fill_rects(file, (struct doomdev_surf_ioctl_fill_rects __user *) arg);
    case DOOMDEV_SURF_IOCTL_DRAW_LINES:
      return surface_draw_lines(file, (struct doomdev_surf_ioctl_draw_lines __user *) arg);
    case DOOMDEV_SURF_IOCTL_DRAW_BACKGROUND:
      return surface_draw_background(file, (struct doomdev_surf_ioctl_draw_background __user *) arg);
    case DOOMDEV_SURF_IOCTL_DRAW_COLUMNS:
      return surface_draw_columns(file, (struct doomdev_surf_ioctl_draw_columns __user *) arg);
    case DOOMDEV_SURF_IOCTL_DRAW_SPANS:
      return surface_draw_spans(file, (struct doomdev_surf_ioctl_draw_spans __user *) arg);
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
      pos = size + filepos;
      break;
    default:
      return -EINVAL;
  }

  file->f_pos = pos;
  return pos;
}

static bool fence_passed(uint32_t fence, uint32_t fence_last) {
  return fence_last >= fence || (fence_last < 0x1000000 && fence > 0x03000000);
}

static ssize_t surface_read(struct file *file, char __user *buf, size_t count, loff_t *filepos) {
  unsigned long err;

  struct surface_prv *prv = (struct surface_prv *) file->private_data;
  size_t copied, size = prv->width * prv->height;

  unsigned long flags;
  uint32_t fence, fence_last;

  if (*filepos > size || *filepos < 0)
    return 0;

  spin_lock_irqsave(&prv->drvdata->fence_lock, flags);
  fence = prv->drvdata->fence;
  iowrite32(HARDDOOM_INTR_FENCE, prv->drvdata->BAR0 + HARDDOOM_INTR);
  iowrite32(prv->drvdata->fence, prv->drvdata->BAR0 + HARDDOOM_FENCE_WAIT);

  fence_last = ioread32(prv->drvdata->BAR0 + HARDDOOM_FENCE_LAST);
  prv->drvdata->fence_last = fence_last;
  spin_unlock_irqrestore(&prv->drvdata->fence_lock, flags);

  if (!fence_passed(fence, fence_last)) {
    do {
      err = wait_event_interruptible_timeout(prv->drvdata->fence_wait, \
          fence_passed(fence, prv->drvdata->fence_last), HZ / 10);
      if (err == -ERESTARTSYS)
        return err;
    } while (err == 0);
  }

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

int surface_get(struct doom_prv *drvdata, struct fd *fd, struct surface_prv **res) {
  struct surface_prv *surface;

  if (!is_surface_fd(fd))
    return -EINVAL;

  surface = (struct surface_prv *) fd->file->private_data;
  if (surface->drvdata != drvdata)
    return -EINVAL;

  *res = surface;
  return 0;
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

long surface_create(struct doom_prv *drvdata, struct doomdev_ioctl_create_surface __user *args) {
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
    .dirty = false,
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
  fdput(surface_fd);
  return fd;

create_getfd_err:
  dma_free_coherent(prv->drvdata->pci, prv->size, prv->surface, prv->surface_dma);
create_allocate_err:
  kfree(prv);
create_kmalloc_err:
  return err;
}

#undef _MUST
