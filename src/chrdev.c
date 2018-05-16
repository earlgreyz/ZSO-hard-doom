#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/ioctl.h>
#include <linux/kernel.h>
#include <linux/module.h>

#include "../include/doomdev.h"
#include "../include/harddoom.h"

#include "chrdev.h"
#include "colormaps.h"
#include "flat.h"
#include "surface.h"
#include "texture.h"

#define MODULE_NAME "harddoom"

static dev_t doom_major;
static struct class doom_class = {
  .owner = THIS_MODULE,
  .name = MODULE_NAME,
};

static int doomdev_open(struct inode *ino, struct file *file) {
  file->private_data = container_of(ino->i_cdev, struct doom_prv, cdev);
  return 0;
}

static int doomdev_release(struct inode *ino, struct file *file) {
  return 0;
}

static long doomdev_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
  struct doom_prv *prv = file->private_data;

  switch (cmd) {
    case DOOMDEV_IOCTL_CREATE_SURFACE:
      return surface_create(prv, (struct doomdev_ioctl_create_surface *) arg);
    case DOOMDEV_IOCTL_CREATE_TEXTURE:
      return texture_create(prv, (struct doomdev_ioctl_create_texture *) arg);
    case DOOMDEV_IOCTL_CREATE_FLAT:
      return flat_create(prv, (struct doomdev_ioctl_create_flat *) arg);
    case DOOMDEV_IOCTL_CREATE_COLORMAPS:
      return colormaps_create(prv, (struct doomdev_ioctl_create_colormaps *) arg);
    default:
      return -ENOTTY;
  }
}

static struct file_operations doom_operations = {
  .owner = THIS_MODULE,
  .unlocked_ioctl = doomdev_ioctl,
  .compat_ioctl = doomdev_ioctl,
  .open = doomdev_open,
  .release = doomdev_release,
};

void doom_cdev_init(struct cdev *cdev, dev_t *dev) {
  cdev_init(cdev, &doom_operations);
  *dev = doom_major; // TODO: allocate new number
}

struct device *doom_device_create(struct device *parent, struct doom_prv *drvdata) {
  return device_create(&doom_class, parent, doom_major, drvdata, "doom%d", 0);
}

void doom_device_destroy(dev_t dev) {
  device_destroy(&doom_class, dev);
}

int doom_chrdev_register_driver(void) {
  unsigned long err;

  err = alloc_chrdev_region(&doom_major, 0, 256, MODULE_NAME);
  if (IS_ERR_VALUE(err)) {
    goto init_chrdev_err;
  }

  err = class_register(&doom_class);
  if (IS_ERR_VALUE(err)) {
    goto init_class_err;
  }

  return 0;

init_class_err:
  unregister_chrdev_region(doom_major, 8);
init_chrdev_err:
  return err;
}

void doom_chrdev_unregister_driver(void) {
  class_unregister(&doom_class);
  unregister_chrdev_region(doom_major, 8);
}
