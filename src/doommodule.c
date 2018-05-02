#include <linux/kernel.h>
#include <linux/module.h>

#include "doompci.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mikolaj Walczak");
MODULE_DESCRIPTION("Doom Device Driver");

static int __init doom_init(void) {
  return doom_pci_register_driver();
}

static void __exit doom_exit(void) {
  doom_pci_unregister_driver();
}

module_init(doom_init);
module_exit(doom_exit);
