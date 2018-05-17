#include <linux/bitmap.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/ioctl.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/spinlock.h>

#include "../include/doomdev.h"
#include "../include/harddoom.h"

#include "chrdev.h"
#include "colormaps.h"
#include "flat.h"
#include "surface.h"
#include "texture.h"

#define MODULE_NAME "harddoom"

#define DOOM_MAX_DEV 256

static dev_t doom_major;

DEFINE_SPINLOCK(bitmap_lock);
static unsigned long doom_bitmap[BITS_TO_LONGS(DOOM_MAX_DEV)];

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

void doom_cdev_init(struct cdev *cdev) {
  cdev_init(cdev, &doom_operations);
}

struct device *doom_device_create(struct device *parent, struct doom_prv *drvdata) {
  int bit;
  struct device *device;

  spin_lock(&bitmap_lock);
  bit = find_first_zero_bit(doom_bitmap, DOOM_MAX_DEV);
  set_bit(bit, doom_bitmap);
  spin_unlock(&bitmap_lock);

  printk(KERN_INFO "[doomdev] Creating device doom%d\n", bit);

  drvdata->dev = doom_major + bit;
  device = device_create(&doom_class, parent, drvdata->dev, drvdata, "doom%d", bit);
  if (IS_ERR(device)) {
    spin_lock(&bitmap_lock);
    clear_bit(bit, doom_bitmap);
    spin_unlock(&bitmap_lock);
  }
  return device;
}

void doom_device_destroy(dev_t dev) {
  int bit = dev - doom_major;

  device_destroy(&doom_class, dev);

  spin_lock(&bitmap_lock);
  clear_bit(bit, doom_bitmap);
  spin_unlock(&bitmap_lock);

  printk(KERN_INFO "[doomdev] Destroying device doom%d\n", bit);
}

int doom_chrdev_register_driver(void) {
  unsigned long err;

  err = alloc_chrdev_region(&doom_major, 0, DOOM_MAX_DEV, MODULE_NAME);
  if (IS_ERR_VALUE(err)) {
    goto init_chrdev_err;
  }

  err = class_register(&doom_class);
  if (IS_ERR_VALUE(err)) {
    goto init_class_err;
  }

  bitmap_zero(doom_bitmap, DOOM_MAX_DEV);
  return 0;

init_class_err:
  unregister_chrdev_region(doom_major, DOOM_MAX_DEV);
init_chrdev_err:
  return err;
}

void doom_chrdev_unregister_driver(void) {
  class_unregister(&doom_class);
  unregister_chrdev_region(doom_major, DOOM_MAX_DEV);
}
