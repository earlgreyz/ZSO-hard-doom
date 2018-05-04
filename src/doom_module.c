#include <linux/kernel.h>
#include <linux/module.h>

#include "doom_pcidrv.h"
#include "doom_chrdev.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mikolaj Walczak");
MODULE_DESCRIPTION("Doom Device Driver");

static int __init doom_init(void) {
  int err;

  if ((err = doom_chrdev_register_driver()))
    return err;

  return doom_pci_register_driver();
}

static void __exit doom_exit(void) {
  doom_chrdev_unregister_driver();
  doom_pci_unregister_driver();
}

module_init(doom_init);
module_exit(doom_exit);
