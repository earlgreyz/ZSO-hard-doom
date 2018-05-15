#include <linux/anon_inodes.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/ioctl.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/vmalloc.h>

#include "../include/harddoom.h"

#include "cmd.h"
#include "pt.h"
#include "surface.h"

#define SURFACE_FILE_TYPE "surface"

#define SURFACE_MAX_SIZE    2048
#define SURFACE_MIN_WIDTH     64
#define SURFACE_MIN_HEIGHT     1
#define SURFACE_WIDTH_MASK  0x7f

struct surface_prv {
  struct doom_prv   *drvdata;

  uint32_t          width;
  uint32_t          height;

  size_t            size;
  void              *surface;
  struct pt_entry   *pt;

  dma_addr_t        surface_dma;
  dma_addr_t        pt_dma;
};

static long surface_select(struct surface_prv *prv) {
  unsigned long err;

  if (prv->drvdata->surface == prv->surface_dma) {
    return 0;
  }

  if ((err = doom_cmd(prv->drvdata, HARDDOOM_CMD_SURF_DST_PT(prv->pt_dma)))) {
    goto surface_surf_src_pt_err;
  }
  if ((err = doom_cmd(prv->drvdata, HARDDOOM_CMD_SURF_DIMS(prv->width, prv->height)))) {
    goto surface_surf_dims_err;
  }

  return 0;

surface_surf_dims_err:
  prv->drvdata->surface = -1;
surface_surf_src_pt_err:
  return -EFAULT;
}

static long surface_copy_rects(struct file *file, struct doomdev_surf_ioctl_copy_rects *args) {
  return -ENOTTY;
}

static long surface_fill_rects(struct file *file, struct doomdev_surf_ioctl_fill_rects *args) {
  unsigned long err;

  long i;
  struct doomdev_fill_rect *rect;
  struct surface_prv *prv = (struct surface_prv *) file->private_data;

  mutex_lock(&prv->drvdata->cmd_mutex);
  if ((err = surface_select(prv))) {
    goto fill_rects_surface_err;
  }

  for (i = 0; i < args->rects_num; ++i) {
    rect = (struct doomdev_fill_rect *) args->rects_ptr + i;
    //printk(KERN_INFO "fill_rects %ld -> fill_color\n", i);
    if ((err = doom_cmd(prv->drvdata, HARDDOOM_CMD_FILL_COLOR(rect->color)))) {
      goto fill_rects_rect_err;
    }
    //printk(KERN_INFO "draw_lines %ld -> xy_a\n", i);
    if ((err = doom_cmd(prv->drvdata, HARDDOOM_CMD_XY_A(rect->pos_dst_x, rect->pos_dst_y)))) {
      goto fill_rects_rect_err;
    }
    //printk(KERN_INFO "draw_lines %ld -> fill_rect\n", i);
    if ((err = doom_cmd(prv->drvdata, HARDDOOM_CMD_FILL_RECT(rect->width, rect->height)))) {
      goto fill_rects_rect_err;
    }
  }

  mutex_unlock(&prv->drvdata->cmd_mutex);
  return 0;

fill_rects_rect_err:
  err = i;// == 0? -EFAULT: i;
fill_rects_surface_err:
  mutex_unlock(&prv->drvdata->cmd_mutex);
  return err;
}

static long surface_draw_lines(struct file *file, struct doomdev_surf_ioctl_draw_lines *args) {
  unsigned long err;

  long i;
  struct doomdev_line *line;
  struct surface_prv *prv = (struct surface_prv *) file->private_data;

  mutex_lock(&prv->drvdata->cmd_mutex);
  if ((err = surface_select(prv))) {
    goto fill_lines_surface_err;
  }

  for (i = 0; i < args->lines_num; ++i) {
    line = (struct doomdev_line *) args->lines_ptr + i;
    //printk(KERN_INFO "draw_lines %ld -> fill_color\n", i);
    if ((err = doom_cmd(prv->drvdata, HARDDOOM_CMD_FILL_COLOR(line->color)))) {
      goto fill_lines_line_err;
    }
    //printk(KERN_INFO "draw_lines %ld -> xy_a\n", i);
    if ((err = doom_cmd(prv->drvdata, HARDDOOM_CMD_XY_A(line->pos_a_x, line->pos_a_y)))) {
      goto fill_lines_line_err;
    }
    //printk(KERN_INFO "draw_lines %ld -> xy_b\n", i);
    if ((err = doom_cmd(prv->drvdata, HARDDOOM_CMD_XY_B(line->pos_b_x, line->pos_b_y)))) {
      goto fill_lines_line_err;
    }
    //printk(KERN_INFO "draw_lines %ld -> draw_line\n", i);
    if ((err = doom_cmd(prv->drvdata, HARDDOOM_CMD_DRAW_LINE))) {
      goto fill_lines_line_err;
    }
  }

  mutex_unlock(&prv->drvdata->cmd_mutex);
  return 0;

fill_lines_line_err:
  err = i; // == 0? -EFAULT: i;
fill_lines_surface_err:
  mutex_unlock(&prv->drvdata->cmd_mutex);
  return err;
}

static long surface_draw_background(struct file *file, struct doomdev_surf_ioctl_draw_background *args) {
  return -ENOTTY;
}

static long surface_draw_columns(struct file *file, struct doomdev_surf_ioctl_draw_columns *args) {
  return -ENOTTY;
}

static long surface_draw_spans(struct file *file, struct doomdev_surf_ioctl_draw_spans *args) {
  return -ENOTTY;
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
  file->f_pos = filepos;
  return 0;
}

static ssize_t surface_read(struct file *file, char __user *buf, size_t count, loff_t *filepos) {
  unsigned long err;

  size_t size, copied;
  struct surface_prv *prv = (struct surface_prv *) file->private_data;

  size = prv->width * prv->height;
  if (*filepos > size || *filepos < 0) {
    return 0;
  }

  if (count > size - *filepos) {
    count = size - *filepos;
  }

  err = copy_to_user(buf, prv->surface + *filepos, count);
  copied = count - err;
  if (copied == 0) {
    return -EFAULT;
  }

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

static int allocate_surface(struct surface_prv *prv, size_t size) {
  long pt_len;
  size_t pt_size;
  size_t aligned_size = ALIGN(size, PT_ALIGNMENT);

  pt_len = pt_length(size);
  if (IS_ERR_VALUE(pt_len)) {
    return pt_len;
  }

  pt_size = pt_len * sizeof(struct pt_entry);
  prv->size = aligned_size + pt_size;

  prv->surface = dma_alloc_coherent(prv->drvdata->pci, prv->size, &prv->surface_dma, GFP_KERNEL);
  if (IS_ERR(prv->surface)) {
    return PTR_ERR(prv->surface);
  }

  prv->pt = (struct pt_entry *) (prv->surface + aligned_size);
  prv->pt_dma = prv->surface_dma + aligned_size;
  pt_fill(prv->surface_dma, prv->pt, pt_len);

  return 0;
}

long surface_create(struct doom_prv *drvdata, struct doomdev_ioctl_create_surface *args) {
  unsigned long err;

  struct surface_prv *prv;
  int surface_fd;
  struct fd fd;

  if (args->width > SURFACE_MAX_SIZE || args->height > SURFACE_MAX_SIZE) {
    return -EOVERFLOW;
  } else if (args->width < SURFACE_MIN_WIDTH
      || (args->width & SURFACE_WIDTH_MASK) != 0
      || args->height < SURFACE_MIN_HEIGHT) {
    return -EINVAL;
  }

  prv = (struct surface_prv *) kmalloc(sizeof(struct surface_prv), GFP_KERNEL);
  if (IS_ERR(prv)) {
    printk(KERN_WARNING "[doom_surface] Surface Create unable to alocate private\n");
    err = PTR_ERR(prv);
    goto create_kmalloc_err;
  }

  *prv = (struct surface_prv){
    .drvdata = drvdata,
    .width = args->width,
    .height = args->height,
  };

  err = allocate_surface(prv, args->width * args->height);
  if (IS_ERR_VALUE(err)) {
    printk(KERN_WARNING "[doom_surface] Surface Create unable to alocate surface\n");
    goto create_allocate_err;
  }

  surface_fd = anon_inode_getfd(SURFACE_FILE_TYPE, &surface_ops, prv, O_RDONLY | O_CLOEXEC);
  if (IS_ERR_VALUE((unsigned long) surface_fd)) {
    printk(KERN_WARNING "[doom_surface] Surface Create unable to alocate a fd\n");
    err = (unsigned long) surface_fd;
    goto create_getfd_err;
  }

  fd = fdget(surface_fd);
  fd.file->f_mode = FMODE_LSEEK | FMODE_PREAD | FMODE_PWRITE;

  return surface_fd;

create_getfd_err:
  dma_free_coherent(prv->drvdata->pci, prv->size, prv->surface, prv->surface_dma);
create_allocate_err:
  kfree(prv);
create_kmalloc_err:
  return err;
}
