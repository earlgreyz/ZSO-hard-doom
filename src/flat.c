#include <linux/anon_inodes.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/ioctl.h>
#include <linux/module.h>

#include "flat.h"

#define FLAT_FILE_TYPE  "flat"

#define FLAT_MAX_SIZE   (4 << 20)
#define FLAT_MAX_HEIGHT     1023

struct flat_prv {
  struct doom_prv   *shared_data;
  void              *data_ptr;
};

static int flat_release(struct inode *ino, struct file *file) {
  struct flat_prv *private_data = (struct flat_prv *) file->private_data;
  kfree(private_data);
  return 0;
}

static struct file_operations flat_ops = {
  .owner = THIS_MODULE,
  .release = flat_release,
};

long flat_create(struct doom_prv *drvdata, struct doomdev_ioctl_create_flat *args) {
  unsigned long err;
  struct flat_prv *private_data;
  int fd;

  private_data = (struct flat_prv *) kmalloc(sizeof(struct flat_prv), GFP_KERNEL);
  if (IS_ERR(private_data)) {
    err = PTR_ERR(private_data);
    goto create_kmalloc_err;
  }

  *private_data = (struct flat_prv){
    .shared_data = drvdata,
    .data_ptr = (void *) args->data_ptr,
  };

  fd = anon_inode_getfd(FLAT_FILE_TYPE, &flat_ops, private_data, O_RDONLY | O_CLOEXEC);
  if (IS_ERR_VALUE((unsigned long) fd)) {
    err = (unsigned long) fd;
    goto create_getfd_err;
  }

  return fd;

create_getfd_err:
  kfree(private_data);
create_kmalloc_err:
  return err;
}
