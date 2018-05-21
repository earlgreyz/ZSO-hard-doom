#include <linux/anon_inodes.h>
#include <linux/fs.h>
#include <linux/ioctl.h>
#include <linux/module.h>
#include <linux/uaccess.h>

#include "flat.h"

#define FLAT_FILE_TYPE  "flat"
#define FLAT_SIZE       0x1000

static int flat_release(struct inode *ino, struct file *file) {
  struct flat_prv *prv = (struct flat_prv *) file->private_data;
  dma_free_coherent(prv->drvdata->pci, FLAT_SIZE, prv->flat, prv->flat_dma);
  kfree(prv);
  return 0;
}

static struct file_operations flat_ops = {
  .owner = THIS_MODULE,
  .release = flat_release,
};

bool is_flat_fd(struct fd *fd) {
  return (fd->file != NULL) && (fd->file->f_op == &flat_ops);
}

int flat_get(struct doom_prv *drvdata, struct fd *fd, struct flat_prv **res) {
  struct flat_prv *flat;

  if (!is_flat_fd(fd))
    return -EINVAL;

  flat = (struct flat_prv *) fd->file->private_data;
  if (flat->drvdata != drvdata)
    return -EINVAL;

  *res = flat;
  return 0;
}

long flat_create(struct doom_prv *drvdata, struct doomdev_ioctl_create_flat *args) {
  long err;

  struct flat_prv *prv;
  int fd;

  prv = (struct flat_prv *) kmalloc(sizeof(struct flat_prv), GFP_KERNEL);
  if (prv == NULL) {
    printk(KERN_WARNING "[doom_flat] flat_create error: kmalloc\n");
    err = -ENOMEM;
    goto create_kmalloc_err;
  }

  *prv = (struct flat_prv){
    .drvdata = drvdata,
  };

  prv->flat = dma_alloc_coherent(prv->drvdata->pci, FLAT_SIZE, &prv->flat_dma, GFP_KERNEL);
  if (prv->flat == NULL) {
    printk(KERN_WARNING "[doom_flat] flat_create error: dma_alloc_coherent\n");
    err = -ENOMEM;
    goto create_allocate_err;
  }

  err = copy_from_user(prv->flat, (void *) args->data_ptr, FLAT_SIZE);
  if (err > 0) {
    printk(KERN_WARNING "[doom_flat] flat_create error: copy_from_user\n");
    err = -EFAULT;
    goto create_copy_err;
  }

  fd = anon_inode_getfd(FLAT_FILE_TYPE, &flat_ops, prv, O_RDONLY);
  if (fd < 0) {
    printk(KERN_WARNING "[doom_flat] flat_create error: anon_inode_getfd\n");
    err = fd;
    goto create_getfd_err;
  }

  return fd;

create_getfd_err:
create_copy_err:
  dma_free_coherent(prv->drvdata->pci, FLAT_SIZE, prv->flat, prv->flat_dma);
create_allocate_err:
  kfree(prv);
create_kmalloc_err:
  return err;
}
