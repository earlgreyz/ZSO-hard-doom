#include <linux/anon_inodes.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/ioctl.h>
#include <linux/module.h>

#include "texture.h"

#define TEXTURE_FILE_TYPE  "texture"

#define TEXTURE_MAX_SIZE   (4 << 20)
#define TEXTURE_MAX_HEIGHT     1023

struct texture_prv {
  struct doom_prv   *shared_data;
  void              *data_ptr;
  uint32_t          size;
  uint16_t          height;
};

static int texture_release(struct inode *ino, struct file *file) {
  struct texture_prv *private_data = (struct texture_prv *) file->private_data;
  kfree(private_data);
  return 0;
}

static struct file_operations texture_ops = {
  .owner = THIS_MODULE,
  .release = texture_release,
};

long texture_create(struct doom_prv *drvdata, struct doomdev_ioctl_create_texture *args) {
  unsigned long err;
  struct texture_prv *private_data;
  int fd;

  if (args->size > TEXTURE_MAX_SIZE || args->height > TEXTURE_MAX_HEIGHT) {
    return -EOVERFLOW;
  }

  private_data = (struct texture_prv *) kmalloc(sizeof(struct texture_prv), GFP_KERNEL);
  if (IS_ERR(private_data)) {
    err = PTR_ERR(private_data);
    goto create_kmalloc_err;
  }

  *private_data = (struct texture_prv){
    .shared_data = drvdata,
    .data_ptr = (void *) args->data_ptr,
    .size = args->size,
    .height = args->height,
  };

  fd = anon_inode_getfd(TEXTURE_FILE_TYPE, &texture_ops, private_data, O_RDONLY | O_CLOEXEC);
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
