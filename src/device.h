#ifndef HARDDOOM_DEVICE_H
#define HARDDOOM_DEVICE_H

#include <linux/cdev.h>
#include <linux/device.h>

#include "private.h"

/**
 * Registers harddoom char device driver.
 * @success returns 0
 * @error returns negated error code.
 **/
int harddoom_chrdev_register_driver(void);

/**
 * Unregisters harddoom char device driver.
 **/
void harddoom_chrdev_unregister_driver(void);

/**
 * Initializes the cdev structs and sets the harddoom file_operations.
 **/
void harddoom_cdev_init(struct cdev *cdev);

/**
 * Finds an unused number, and creates a harddoom device.
 * The device struct can be accessed via drvdata->device.
 * The allocated dev_t can be accessed via drvdata->dev.
 * @success returns 0
 * @failure returns a negated error code.
 **/
int harddoom_device_create(struct device *parent, struct doom_prv *drvdata);

/**
 * Destroys a device of a given dev_t number.
 **/
void harddoom_device_destroy(dev_t dev);

#endif
