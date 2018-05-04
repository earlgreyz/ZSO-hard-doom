#ifndef DOOM_CHRDEV_H
#define DOOM_CHRDEV_H

#include <linux/device.h>
#include <linux/cdev.h>

int doom_chrdev_register_driver(void);
void doom_chrdev_unregister_driver(void);

struct cdev *doom_cdev_alloc(void);
struct device *doom_device_create(struct device *parent);
void doom_device_destroy(struct device *dev);

#endif
