#include <linux/kernel.h>
#include <linux/module.h>

#include "device.h"
#include "pci.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mikolaj Walczak");
MODULE_DESCRIPTION("HardDoom Device Driver");

static int __init doom_init(void) {
  int err;

  if ((err = harddoom_chrdev_register_driver())) {
    return err;
  }

  return harddoom_pci_register_driver();
}

static void __exit doom_exit(void) {
  harddoom_pci_unregister_driver();
  harddoom_chrdev_unregister_driver();
}

module_init(doom_init);
module_exit(doom_exit);
