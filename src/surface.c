#include <linux/anon_inodes.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/ioctl.h>
#include <linux/module.h>
#include <linux/vmalloc.h>

#include "surface.h"

#define SURFACE_FILE_TYPE "surface"

#define SURFACE_MAX_SIZE    2048
#define SURFACE_MIN_WIDTH     64
#define SURFACE_MIN_HEIGHT     1
#define SURFACE_WIDTH_MASK  0x7f

struct surface_prv {
  struct doom_prv   *shared_data;
  void              *surface;
};

static long surface_copy_rects(struct file *file, struct doomdev_surf_ioctl_copy_rects *args) {
  return -ENOTTY;
}

static long surface_fill_rects(struct file *file, struct doomdev_surf_ioctl_fill_rects *args) {
  return -ENOTTY;
}

static long surface_draw_lines(struct file *file, struct doomdev_surf_ioctl_draw_lines *args) {
  return -ENOTTY;
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
  return -ENOTTY;
}

ssize_t surface_read(struct file *file, char __user *buf, size_t count, loff_t *filepos) {
  return -ENOTTY;
}

static int surface_release(struct inode *ino, struct file *file) {
  struct surface_prv *private_data = (struct surface_prv *) file->private_data;
  vfree(private_data->surface);
  kfree(private_data);
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

long surface_create(struct doom_prv *drvdata, struct doomdev_ioctl_create_surface *args) {
  unsigned long err;
  struct surface_prv *private_data;
  int surface_fd;
  struct fd fd;

  if (args->width > SURFACE_MAX_SIZE || args->height > SURFACE_MAX_SIZE) {
    return -EOVERFLOW;
  } else if (args->width < SURFACE_MIN_WIDTH
      || (args->width & SURFACE_WIDTH_MASK) != 0
      || args->height < SURFACE_MIN_HEIGHT) {
    return -EINVAL;
  }

  private_data = (struct surface_prv *) kmalloc(sizeof(struct surface_prv), GFP_KERNEL);
  if (IS_ERR(private_data)) {
    err = PTR_ERR(private_data);
    goto create_kmalloc_err;
  }

  private_data->shared_data = drvdata;

  private_data->surface = vmalloc(args->width * args->height);
  if (IS_ERR(private_data->surface)) {
    err = PTR_ERR(private_data->surface);
    goto create_vmalloc_err;
  }

  surface_fd = anon_inode_getfd(SURFACE_FILE_TYPE, &surface_ops, private_data, O_RDONLY | O_CLOEXEC);

  if (IS_ERR_VALUE((unsigned long) surface_fd)) {
    err = (unsigned long) surface_fd;
    goto create_getfd_err;
  }

  fd = fdget(surface_fd);
  fd.file->f_mode = FMODE_LSEEK | FMODE_PREAD | FMODE_PWRITE;

  return surface_fd;

create_getfd_err:
  vfree(private_data->surface);
create_vmalloc_err:
  kfree(private_data);
create_kmalloc_err:
  return err;
}
