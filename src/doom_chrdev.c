#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/ioctl.h>
#include <linux/device.h>
#include <linux/cdev.h>

#include "doom_chrdev.h"

static dev_t doom_major;
static struct class doom_class = {
	.name = "doom",
	.owner = THIS_MODULE,
};

int doom_chrdev_register_driver(void) {
  unsigned long err;

  err = class_register(&doom_class);
  if (IS_ERR_VALUE(err)) {
		goto init_class_err;
  }

  err = alloc_chrdev_region(&doom_major, 0, 8, "doom");
  if (IS_ERR_VALUE(err)) {
		goto init_chrdev_err;
  }

  return 0;

init_chrdev_err:
  class_unregister(&doom_class);
init_class_err:
  return err;
}

void doom_chrdev_unregister_driver(void) {
  unregister_chrdev_region(doom_major, 8);
  class_unregister(&doom_class);
}
