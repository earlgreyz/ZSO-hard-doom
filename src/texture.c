#include <linux/anon_inodes.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/ioctl.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/vmalloc.h>

#include "pt.h"
#include "texture.h"

#define TEXTURE_FILE_TYPE  "texture"

#define TEXTURE_MAX_SIZE   (4 << 20)
#define TEXTURE_MAX_HEIGHT     1023

struct texture_prv {
  struct doom_prv   *shared_data;
  void              *texture;
  struct pt_entry   *pt;
  uint16_t          height;
};

static int texture_release(struct inode *ino, struct file *file) {
  struct texture_prv *private_data = (struct texture_prv *) file->private_data;
  vfree(private_data->texture);
  kfree(private_data);
  return 0;
}

static struct file_operations texture_ops = {
  .owner = THIS_MODULE,
  .release = texture_release,
};

static int allocate_texture(struct texture_prv *private_data, size_t size) {
  long pt_len;
  size_t pt_size;
  size_t aligned_size = ALIGN(size, PT_ALIGNMENT);

  printk(KERN_DEBUG "[doom_texture] %lu aligned to %lu\n", size, aligned_size);

  pt_len = pt_length(size);
  if (IS_ERR_VALUE(pt_len)) {
    return pt_len;
  }

  pt_size = pt_len * sizeof(struct pt_entry);
  private_data->texture = vmalloc_32(aligned_size + pt_size);
  if (IS_ERR(private_data->texture)) {
    return PTR_ERR(private_data->texture);
  }

  private_data->pt = (struct pt_entry *) (private_data->texture + aligned_size);
  pt_fill(private_data->texture, private_data->pt, pt_len);
  return 0;
}

long texture_create(struct doom_prv *drvdata, struct doomdev_ioctl_create_texture *args) {
  unsigned long err;

  struct texture_prv *private_data;
  int fd;

  if (args->size > TEXTURE_MAX_SIZE || args->height > TEXTURE_MAX_HEIGHT) {
    return -EOVERFLOW;
  }

  private_data = (struct texture_prv *) kmalloc(sizeof(struct texture_prv), GFP_KERNEL);
  if (IS_ERR(private_data)) {
    printk(KERN_WARNING "[doom_surface] Texture Create unable to alocate private\n");
    err = PTR_ERR(private_data);
    goto create_kmalloc_err;
  }

  *private_data = (struct texture_prv){
    .shared_data = drvdata,
    .height = args->height,
  };

  err = allocate_texture(private_data, args->size);
  if (IS_ERR_VALUE(err)) {
    printk(KERN_WARNING "[doom_surface] Texture Create unable to alocate private\n");
    goto create_allocate_err;
  }

  err = copy_from_user(private_data->texture, (void __user *) args->data_ptr, args->size);
  if (IS_ERR_VALUE(err)) {
    printk(KERN_WARNING "[doom_surface] Texture Create unable to copy texture data\n");
    goto create_copy_err;
  }

  fd = anon_inode_getfd(TEXTURE_FILE_TYPE, &texture_ops, private_data, O_RDONLY | O_CLOEXEC);
  if (IS_ERR_VALUE((unsigned long) fd)) {
    printk(KERN_WARNING "[doom_surface] Surface Create unable to alocate a fd\n");
    err = (unsigned long) fd;
    goto create_getfd_err;
  }

  return fd;

create_getfd_err:
create_copy_err:
  vfree(private_data->texture);
create_allocate_err:
  kfree(private_data);
create_kmalloc_err:
  return err;
}
