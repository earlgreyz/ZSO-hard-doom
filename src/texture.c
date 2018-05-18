#include <linux/anon_inodes.h>
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
#define TEXTURE_ALIGNMENT     0x100

static int texture_release(struct inode *ino, struct file *file) {
  struct texture_prv *prv = (struct texture_prv *) file->private_data;
  dma_free_coherent(prv->drvdata->pci, prv->size, prv->texture, prv->texture_dma);
  kfree(prv);
  return 0;
}

static struct file_operations texture_ops = {
  .owner = THIS_MODULE,
  .release = texture_release,
};

bool is_texture_fd(struct fd *fd) {
  return (fd->file != NULL) && (fd->file->f_op == &texture_ops);
}

int texture_get(struct doom_prv *drvdata, int fd, struct texture_prv **res) {
  struct fd texture_fd;
  struct texture_prv *texture;

  texture_fd = fdget(fd);
  if (!is_texture_fd(&texture_fd))
    return -EINVAL;

  texture = (struct texture_prv *) texture_fd.file->private_data;
  if (texture->drvdata != drvdata)
    return -EINVAL;

  *res = texture;
  return 0;
}

static int allocate_texture(struct texture_prv *prv, size_t size) {
  long pt_len;
  size_t pt_size;
  size_t aligned_size = ALIGN(size, PT_ALIGNMENT);

  if ((pt_len = pt_length(size)) <= 0) {
    return pt_len;
  }

  pt_size = pt_len * sizeof(struct pt_entry);
  prv->size = aligned_size + pt_size;

  prv->texture = dma_alloc_coherent(prv->drvdata->pci, prv->size, &prv->texture_dma, GFP_KERNEL);
  if (prv->texture == NULL) {
    return -ENOMEM;
  }

  prv->pt = (struct pt_entry *) (prv->texture + aligned_size);
  prv->pt_dma = prv->texture_dma + aligned_size;
  pt_fill(prv->texture_dma, prv->pt, pt_len);
  return 0;
}

long texture_create(struct doom_prv *drvdata, struct doomdev_ioctl_create_texture *args) {
  long err;

  struct texture_prv *prv;
  int fd;
  size_t size;

  if (args->size > TEXTURE_MAX_SIZE || args->height > TEXTURE_MAX_HEIGHT) {
    return -EOVERFLOW;
  }

  prv = (struct texture_prv *) kmalloc(sizeof(struct texture_prv), GFP_KERNEL);
  if (prv == NULL) {
    printk(KERN_WARNING "[doom_texture] texture_create error: kmalloc\n");
    err = -ENOMEM;
    goto create_kmalloc_err;
  }

  size = ALIGN(args->size, TEXTURE_ALIGNMENT);

  *prv = (struct texture_prv){
    .drvdata = drvdata,
    .height = args->height,
    .size_m1 = (size >> 8) - 1,
  };

  if ((err = allocate_texture(prv, size))) {
    printk(KERN_WARNING "[doom_texture] texture_create error: allocate_texture\n");
    goto create_allocate_err;
  }

  err = copy_from_user(prv->texture, (void __user *) args->data_ptr, args->size);
  if (err > 0) {
    printk(KERN_WARNING "[doom_texture] texture_create error: copy_from_user\n");
    err = -EFAULT;
    goto create_copy_err;
  }

  memset(prv->texture + args->size, 0, size - args->size);

  fd = anon_inode_getfd(TEXTURE_FILE_TYPE, &texture_ops, prv, O_RDONLY);
  if (fd < 0) {
    printk(KERN_WARNING "[doom_texture] texture_create error: anon_inode_getfd\n");
    err = fd;
    goto create_getfd_err;
  }

  return fd;

create_getfd_err:
create_copy_err:
  dma_free_coherent(prv->drvdata->pci, prv->size, prv->texture, prv->texture_dma);
create_allocate_err:
  kfree(prv);
create_kmalloc_err:
  return err;
}
