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
  void *surface = vmalloc(width * height);
  if (IS_ERR(surface)) {
    return PTR_ERR(surface);
  }
  return anon_inode_getfd("surface", &surface_ops, surface, 0);
}
