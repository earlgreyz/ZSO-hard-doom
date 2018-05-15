#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/ioctl.h>
#include <linux/kernel.h>
#include <linux/module.h>

#include "../include/doomdev.h"
#include "../include/harddoom.h"

#include "chrdev.h"
#include "flat.h"
#include "surface.h"
#include "texture.h"

#define MODULE_NAME "harddoom"

static dev_t doom_major;
static struct class doom_class = {
  .owner = THIS_MODULE,
  .name = MODULE_NAME,
};

struct device_entry {
  struct list_head list;
  dev_t dev;
  struct doom_prv *drvdata;
};

LIST_HEAD(device_list);

static int doomdev_open(struct inode *ino, struct file *file) {
  struct device_entry *entry;

  list_for_each_entry(entry, &device_list, list) {
    if (entry->dev == ino->i_cdev->dev) {
      file->private_data = entry->drvdata;
      return 0;
    }
  }

  printk(KERN_WARNING "[doomdev] Unable to find an entry for device %x\n", ino->i_cdev->dev);
  return -EFAULT;
}

static int doomdev_release(struct inode *ino, struct file *file) {
  return 0;
}

static long doomdev_create_colormaps(struct file *file, struct doomdev_ioctl_create_colormaps *arg) {
  return -ENOTTY;
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

struct cdev *doom_cdev_alloc(dev_t *dev) {
  // TODO: different numbers
  struct cdev *doom_dev = cdev_alloc();
  if (!IS_ERR(doom_dev)) {
    doom_dev->ops = &doom_operations;
    doom_dev->owner = THIS_MODULE;
  }
  *dev = doom_major;
  return doom_dev;
}

struct device *doom_device_create(struct device *parent, struct doom_prv *drvdata) {
  struct device *device;
  struct device_entry *entry;

  device = device_create(&doom_class, parent, doom_major, drvdata, "doom%d", 0);
  if (IS_ERR(device)) {
    printk(KERN_ERR "[doomdev] Doom device create error: device_create\n");
    return device;
  }

  entry = kmalloc(sizeof(struct device_entry), GFP_KERNEL);
  if (IS_ERR(entry)) {
    printk(KERN_ERR "[doomdev] Doom device create error: kmalloc\n");
    return (struct device *) entry;
  }

  entry->dev = device->devt;
  entry->drvdata = drvdata;
  INIT_LIST_HEAD(&entry->list);

  list_add(&entry->list, &device_list);
  return device;
}

void doom_device_destroy(dev_t dev) {
  struct device_entry *entry, *tmp;

  list_for_each_entry_safe(entry, tmp, &device_list, list) {
    if (entry->dev == dev) {
      list_del(&entry->list);
      kfree(entry);
      break;
    }
  }

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
