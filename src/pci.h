#ifndef HARDDOOM_PCI_H
#define HARDDOOM_PCI_H

/**
 * Registers a pci driver for a harddoom device.
 * @success returns 0
 * @error returns negated error code.
 **/
int harddoom_pci_register_driver(void);

/**
 * Unregisters harddoom pci driver.
 **/
void harddoom_pci_unregister_driver(void);

#endif
