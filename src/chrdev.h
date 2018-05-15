#ifndef HARDDOOM_CHRDEV_H
#define HARDDOOM_CHRDEV_H

#include <linux/cdev.h>
#include <linux/device.h>

#include "private.h"

int doom_chrdev_register_driver(void);
void doom_chrdev_unregister_driver(void);

void doom_cdev_init(struct cdev *cdev, dev_t *dev);

struct device *doom_device_create(struct device *parent, struct doom_prv *drvdata);
void doom_device_destroy(dev_t dev);

#endif
