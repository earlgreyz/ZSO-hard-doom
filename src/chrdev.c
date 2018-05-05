#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/ioctl.h>
#include <linux/kernel.h>
#include <linux/module.h>

#include "../include/doomdev.h"

#include "chrdev.h"
#include "surface.h"

#define MODULE_NAME "harddoom"

static dev_t doom_major;
static struct class doom_class = {
  .owner = THIS_MODULE,
	.name = MODULE_NAME,
};

static int doomdev_open(struct inode *ino, struct file *file) {
	return 0;
}

static int doomdev_release(struct inode *ino, struct file *file) {
	return 0;
}

static long doomdev_create_surface(struct file *file, struct doomdev_ioctl_create_surface *arg) {
  return doomsurf_create((struct doom_prv *) file->private_data, arg->width, arg->height);
}

static long doomdev_create_texture(struct file *file, struct doomdev_ioctl_create_texture *arg) {
  return -ENOTTY;
}

static long doomdev_create_flat(struct file *file, struct doomdev_ioctl_create_flat *arg) {
  return -ENOTTY;
}

static long doomdev_create_colormaps(struct file *file, struct doomdev_ioctl_create_colormaps *arg) {
  return -ENOTTY;
}

static long doomdev_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
  switch (cmd) {
    case DOOMDEV_IOCTL_CREATE_SURFACE:
      return doomdev_create_surface(file, (struct doomdev_ioctl_create_surface *) arg);
    case DOOMDEV_IOCTL_CREATE_TEXTURE:
      return doomdev_create_texture(file, (struct doomdev_ioctl_create_texture *) arg);
    case DOOMDEV_IOCTL_CREATE_FLAT:
      return doomdev_create_flat(file, (struct doomdev_ioctl_create_flat *) arg);
    case DOOMDEV_IOCTL_CREATE_COLORMAPS:
      return doomdev_create_colormaps(file, (struct doomdev_ioctl_create_colormaps *) arg);
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

struct cdev *doom_cdev_alloc(void) {
	struct cdev *doom_dev = cdev_alloc();
	if (!IS_ERR(doom_dev)) {
		doom_dev->ops = &doom_operations;
	}
	return doom_dev;
}

struct device *doom_device_create(dev_t *dev, struct device *parent, struct doom_prv *drvdata) {
	// TODO: different numbers and version
  *dev = doom_major;
	return device_create(&doom_class, parent, doom_major, drvdata, "doom0");
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
