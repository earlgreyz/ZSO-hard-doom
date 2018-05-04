#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/ioctl.h>
#include <linux/device.h>
#include <linux/cdev.h>

#include "doom_chrdev.h"

static dev_t doom_major;
static struct class doom_class = {
  .owner = THIS_MODULE,
	.name = "doom",
};

static int doom_open(struct inode *ino, struct file *filep) {
	return 0;
}

static int doom_release(struct inode *ino, struct file *filep) {
	return 0;
}

static long doom_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
	return -ENOTTY;
}

static struct file_operations doom_operations = {
  .owner = THIS_MODULE,
  .unlocked_ioctl = doom_ioctl,
	.compat_ioctl = doom_ioctl,
  .open = doom_open,
  .release = doom_release,
};

struct cdev *doom_cdev_alloc(void) {
	struct cdev *doom_dev = cdev_alloc();
	if (!IS_ERR(doom_dev)) {
		doom_dev->ops = &doom_operations;
	}
	return doom_dev;
}

struct device *doom_device_create(struct device *parent) {
	// TODO: different numbers and version
	return device_create(&doom_class, parent, doom_major, NULL, "doom0");
}

void doom_device_destroy(struct device *dev) {
	device_destroy(&doom_class, doom_major);
}

int doom_chrdev_register_driver(void) {
  unsigned long err;

  err = alloc_chrdev_region(&doom_major, 0, 8, "doom");
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
