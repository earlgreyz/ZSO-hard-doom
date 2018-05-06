#include <linux/file.h>
#include <linux/fs.h>
#include <linux/anon_inodes.h>
#include <linux/module.h>
#include <linux/ioctl.h>
#include <linux/vmalloc.h>

#include "surface.h"
#include "../include/doomdev.h"

#define SURF_FILE_TYPE "surf"

#define SURF_MAX_SIZE    2048
#define SURF_MIN_WIDTH     64
#define SURF_MIN_HEIGHT     1
#define SURF_WIDTH_MASK  0x7f

struct doomsurf_prv {
  struct doom_prv   *shared_data;
  void              *surface;
};

static long doomsurf_copy_rects(struct file *file, struct doomdev_surf_ioctl_copy_rects *arg) {
  return -ENOTTY;
}

static long doomsurf_fill_rects(struct file *file, struct doomdev_surf_ioctl_fill_rects *arg) {
  return -ENOTTY;
}

static long doomsurf_draw_lines(struct file *file, struct doomdev_surf_ioctl_draw_lines *arg) {
  return -ENOTTY;
}

static long doomsurf_draw_background(struct file *file, struct doomdev_surf_ioctl_draw_background *arg) {
  return -ENOTTY;
}

static long doomsurf_draw_columns(struct file *file, struct doomdev_surf_ioctl_draw_columns *arg) {
  return -ENOTTY;
}

static long doomsurf_draw_spans(struct file *file, struct doomdev_surf_ioctl_draw_spans *arg) {
  return -ENOTTY;
}

static long doomsurf_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
  switch (cmd) {
    case DOOMDEV_SURF_IOCTL_COPY_RECTS:
      return doomsurf_copy_rects(file, (struct doomdev_surf_ioctl_copy_rects *) arg);
    case DOOMDEV_SURF_IOCTL_FILL_RECTS:
      return doomsurf_fill_rects(file, (struct doomdev_surf_ioctl_fill_rects *) arg);
    case DOOMDEV_SURF_IOCTL_DRAW_LINES:
      return doomsurf_draw_lines(file, (struct doomdev_surf_ioctl_draw_lines *) arg);
    case DOOMDEV_SURF_IOCTL_DRAW_BACKGROUND:
      return doomsurf_draw_background(file, (struct doomdev_surf_ioctl_draw_background *) arg);
    case DOOMDEV_SURF_IOCTL_DRAW_COLUMNS:
      return doomsurf_draw_columns(file, (struct doomdev_surf_ioctl_draw_columns *) arg);
    case DOOMDEV_SURF_IOCTL_DRAW_SPANS:
      return doomsurf_draw_spans(file, (struct doomdev_surf_ioctl_draw_spans *) arg);
    default:
      return -ENOTTY;
  }
}

static loff_t doomsurf_llseek(struct file *file, loff_t filepos, int whence) {
  return -ENOTTY;
}

ssize_t doomsurf_read(struct file *file, char __user *buf, size_t count, loff_t *filepos) {
  return -ENOTTY;
}

static int doomsurf_release(struct inode *ino, struct file *file) {
  struct doomsurf_prv *private_data = (struct doomsurf_prv *) file->private_data;
  vfree(private_data->surface);
  kfree(private_data);
  return 0;
}

static struct file_operations surface_ops = {
  .owner = THIS_MODULE,
  .unlocked_ioctl = doomsurf_ioctl,
	.compat_ioctl = doomsurf_ioctl,
  .llseek = doomsurf_llseek,
  .read = doomsurf_read,
  .release = doomsurf_release,
};

long doomsurf_create(struct doom_prv *drvdata, uint16_t width, uint16_t height) {
  unsigned long err;
  int fd;
  struct fd fd_struct;
  struct doomsurf_prv *private_data;

  if (width > SURF_MAX_SIZE || height > SURF_MAX_SIZE) {
    return -EOVERFLOW;
  } else if (width < SURF_MIN_WIDTH || (width & SURF_WIDTH_MASK) != 0
      || height < SURF_MIN_HEIGHT) {
    return -EINVAL;
  }

  private_data = (struct doomsurf_prv *) kmalloc(sizeof(struct doomsurf_prv), GFP_KERNEL);
  if (IS_ERR(private_data)) {
    err = PTR_ERR(private_data);
    goto create_kmalloc_err;
  }

  private_data->shared_data = drvdata;

  private_data->surface = vmalloc(width * height);
  if (IS_ERR(private_data->surface)) {
    err = PTR_ERR(private_data->surface);
    goto create_vmalloc_err;
  }

  fd = anon_inode_getfd(SURF_FILE_TYPE, &surface_ops, private_data, O_RDONLY | O_CLOEXEC);
  if (IS_ERR_VALUE((unsigned long) fd)) {
    err = (unsigned long) fd;
    goto create_getfd_err;
  }

  fd_struct = fdget(fd);
  fd_struct.file->f_mode = FMODE_LSEEK | FMODE_PREAD | FMODE_PWRITE;

  return fd;

create_getfd_err:
  vfree(private_data->surface);
create_vmalloc_err:
  kfree(private_data);
create_kmalloc_err:
  return err;
}
