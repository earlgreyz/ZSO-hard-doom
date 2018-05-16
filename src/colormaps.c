#include <linux/anon_inodes.h>
#include <linux/fs.h>
#include <linux/ioctl.h>
#include <linux/module.h>
#include <linux/uaccess.h>

#include "colormaps.h"

#define COLORMAPS_FILE_TYPE  "colormaps"

#define COLORMAPS_MAX_LENGTH  0x100

static int colormaps_release(struct inode *ino, struct file *file) {
  struct colormaps_prv *prv = (struct colormaps_prv *) file->private_data;
  dma_free_coherent(prv->drvdata->pci, prv->size, prv->colormaps, prv->colormaps_dma);
  kfree(prv);
  return 0;
}

static struct file_operations colormaps_ops = {
  .owner = THIS_MODULE,
  .release = colormaps_release,
};

bool is_colormaps_fd(struct fd *fd) {
  return (fd->file != NULL) && (fd->file->f_op == &colormaps_ops);
}

long colormaps_create(struct doom_prv *drvdata, struct doomdev_ioctl_create_colormaps *args) {
  unsigned long err;
  struct colormaps_prv *prv;
  int fd;

  if (args->num > COLORMAPS_MAX_LENGTH) {
    return -EOVERFLOW;
  }

  prv = (struct colormaps_prv *) kmalloc(sizeof(struct colormaps_prv), GFP_KERNEL);
  if (IS_ERR(prv)) {
    printk(KERN_WARNING "[doom_colormaps] colormaps_create error: kmalloc\n");
    err = PTR_ERR(prv);
    goto create_kmalloc_err;
  }

  *prv = (struct colormaps_prv){
    .drvdata = drvdata,
    .num = args->num,
    .size = COLORMAP_SIZE * args->num,
  };

  prv->colormaps = dma_alloc_coherent(prv->drvdata->pci, prv->size, &prv->colormaps_dma, GFP_KERNEL);
  if (IS_ERR(prv->colormaps)) {
    printk(KERN_WARNING "[doom_colormaps] colormaps_create error: dma_alloc_coherent\n");
    err = PTR_ERR(prv->colormaps);
    goto create_allocate_err;
  }

  err = copy_from_user(prv->colormaps, (void *) args->data_ptr, prv->size);
  if (err > 0) {
    printk(KERN_WARNING "[doom_colormaps] colormaps_create error: copy_from_user\n");
    err = -EFAULT;
    goto create_copy_err;
  }

  fd = anon_inode_getfd(COLORMAPS_FILE_TYPE, &colormaps_ops, prv, O_RDONLY);
  if (IS_ERR_VALUE((unsigned long) fd)) {
    printk(KERN_WARNING "[doom_colormaps] colormaps_create error: anon_inode_getfd\n");
    err = (unsigned long) fd;
    goto create_getfd_err;
  }

  return fd;

create_getfd_err:
create_copy_err:
  dma_free_coherent(prv->drvdata->pci, prv->size, prv->colormaps, prv->colormaps_dma);
create_allocate_err:
  kfree(prv);
create_kmalloc_err:
  return err;
}
