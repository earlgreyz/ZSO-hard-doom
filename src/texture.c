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
  struct doom_prv   *drvdata;

  uint16_t          height;

  size_t            size;
  void              *texture;
  struct pt_entry   *pt;

  dma_addr_t        texture_dma;
  dma_addr_t        pt_dma;
};

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

static int allocate_texture(struct texture_prv *prv, size_t size) {
  long pt_len;
  size_t pt_size;
  size_t aligned_size = ALIGN(size, PT_ALIGNMENT);

  printk(KERN_DEBUG "[doom_texture] %lu aligned to %lu\n", size, aligned_size);

  pt_len = pt_length(size);
  if (IS_ERR_VALUE(pt_len)) {
    return pt_len;
  }

  pt_size = pt_len * sizeof(struct pt_entry);
  prv->size = aligned_size + pt_size;

  prv->texture = dma_alloc_coherent(prv->drvdata->pci, prv->size, &prv->texture_dma, GFP_KERNEL);
  if (IS_ERR(prv->texture)) {
    return PTR_ERR(prv->texture);
  }

  prv->pt = (struct pt_entry *) (prv->texture + aligned_size);
  prv->pt_dma = prv->texture_dma + aligned_size;
  pt_fill(prv->texture, prv->pt, pt_len);
  return 0;
}

long texture_create(struct doom_prv *drvdata, struct doomdev_ioctl_create_texture *args) {
  unsigned long err;

  struct texture_prv *prv;
  int fd;

  if (args->size > TEXTURE_MAX_SIZE || args->height > TEXTURE_MAX_HEIGHT) {
    return -EOVERFLOW;
  }

  prv = (struct texture_prv *) kmalloc(sizeof(struct texture_prv), GFP_KERNEL);
  if (IS_ERR(prv)) {
    printk(KERN_WARNING "[doom_surface] Texture Create unable to alocate private\n");
    err = PTR_ERR(prv);
    goto create_kmalloc_err;
  }

  *prv = (struct texture_prv){
    .drvdata = drvdata,
    .height = args->height,
  };

  err = allocate_texture(prv, args->size);
  if (IS_ERR_VALUE(err)) {
    printk(KERN_WARNING "[doom_surface] Texture Create unable to alocate private\n");
    goto create_allocate_err;
  }

  err = copy_from_user(prv->texture, (void __user *) args->data_ptr, args->size);
  if (IS_ERR_VALUE(err)) {
    printk(KERN_WARNING "[doom_surface] Texture Create unable to copy texture data\n");
    goto create_copy_err;
  }

  fd = anon_inode_getfd(TEXTURE_FILE_TYPE, &texture_ops, prv, O_RDONLY | O_CLOEXEC);
  if (IS_ERR_VALUE((unsigned long) fd)) {
    printk(KERN_WARNING "[doom_surface] Surface Create unable to alocate a fd\n");
    err = (unsigned long) fd;
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
