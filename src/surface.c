#include <linux/file.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/ioctl.h>
#include <linux/anon_inodes.h>
#include <linux/vmalloc.h>

#include "surface.h"


static long doomsurf_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
  return -ENOTTY;
}

static loff_t doomsurf_llseek(struct file *file, loff_t filepos, int whence) {
  return -ENOTTY;
}

ssize_t doomsurf_read(struct file *file, char __user *buf, size_t count, loff_t *filepos) {
  return -ENOTTY;
}

static int doomsurf_release(struct inode *ino, struct file *file) {
  vfree(file->private_data);
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

long doomsurf_create(uint16_t width, uint16_t height) {
  unsigned int err;
  int fd;
  struct file *file;
  void *surface;

  surface = vmalloc(width * height);
  if (IS_ERR(surface)) {
    err = PTR_ERR(surface);
    goto create_vmalloc_err;
  }

  fd = anon_inode_getfd("surface", &surface_ops, surface, O_RDONLY | O_CLOEXEC);
  if (IS_ERR_VALUE(fd)) {
    err = fd;
    goto create_getfd_err;
  }

  file = fget(fd);
  if (IS_ERR(file)) {
    err = PTR_ERR(file);
    goto create_getfile_err;
  }

  file->f_mode = FMODE_LSEEK | FMODE_PREAD | FMODE_PWRITE;

  return fd;

create_getfile_err:
  close(fd);
create_getfd_err:
  vfree(surface);
create_vmalloc_err:
  return err;
}
