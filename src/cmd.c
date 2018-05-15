#include <linux/pci.h>
#include <linux/spinlock.h>

#include "../include/harddoom.h"

#include "cmd.h"

long doom_cmd(struct doom_prv *drvdata, uint32_t cmd) {
  unsigned int fifo_free;
  unsigned long flags;

  spin_lock_irqsave(&drvdata->fifo_lock, flags);

  //printk(KERN_INFO "Waiting to send: %x\n", cmd);
  do {
    fifo_free = ioread32(drvdata->BAR0 + HARDDOOM_FIFO_FREE);
  } while (fifo_free < 1);
  iowrite32(cmd, drvdata->BAR0 + HARDDOOM_FIFO_SEND);
  //printk(KERN_INFO "Command sent to the harddoom: %x\n", cmd);

  spin_unlock_irqrestore(&drvdata->fifo_lock, flags);
  return 0;
}
