#include <linux/anon_inodes.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/ioctl.h>
#include <linux/module.h>
#include <linux/uaccess.h>

#include "flat.h"

#define FLAT_FILE_TYPE  "flat"
#define FLAT_SIZE  0x1000

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

bool is_flat_fd(int fd) {
  struct fd fd_struct = fdget(fd);
  return fd_struct.file->f_op == &flat_ops;
}

long flat_create(struct doom_prv *drvdata, struct doomdev_ioctl_create_flat *args) {
  unsigned long err;
  struct flat_prv *prv;
  int fd;

  prv = (struct flat_prv *) kmalloc(sizeof(struct flat_prv), GFP_KERNEL);
  if (IS_ERR(prv)) {
    printk(KERN_WARNING "[doom_flat] flat_create error: kmalloc\n");
    err = PTR_ERR(prv);
    goto create_kmalloc_err;
  }

  *prv = (struct flat_prv){
    .drvdata = drvdata,
  };

  prv->flat = dma_alloc_coherent(prv->drvdata->pci, FLAT_SIZE, &prv->flat_dma, GFP_KERNEL);
  if (IS_ERR(prv->flat)) {
    printk(KERN_WARNING "[doom_flat] flat_create error: dma_alloc_coherent\n");
    err = PTR_ERR(prv->flat);
    goto create_allocate_err;
  }

  err = copy_from_user(prv->flat, (void *) args->data_ptr, FLAT_SIZE);
  if (err > 0) {
    printk(KERN_WARNING "[doom_flat] flat_create error: copy_from_user\n");
    err = -EFAULT;
    goto create_copy_err;
  }

  fd = anon_inode_getfd(FLAT_FILE_TYPE, &flat_ops, prv, O_RDONLY);
  if (IS_ERR_VALUE((unsigned long) fd)) {
    printk(KERN_WARNING "[doom_flat] flat_create error: anon_inode_getfd\n");
    err = (unsigned long) fd;
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
