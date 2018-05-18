#include <linux/pci.h>
#include <linux/spinlock.h>

#include "../include/harddoom.h"

#include "cmd.h"

#define DOOM_FIFO_SIZE 512
#define DOOM_PING_FREQ (DOOM_FIFO_SIZE / 4)

static void fifo_wait(struct doom_prv *drvdata) {
  unsigned int fifo_free, enabled;

  iowrite32(HARDDOOM_INTR_PONG_ASYNC, drvdata->BAR0 + HARDDOOM_INTR);

  fifo_free = ioread32(drvdata->BAR0 + HARDDOOM_FIFO_FREE);
  if (fifo_free > 0)
    return;

  enabled = ioread32(drvdata->BAR0 + HARDDOOM_INTR_ENABLE);
  enabled |= HARDDOOM_INTR_PONG_ASYNC;
  iowrite32(enabled, drvdata->BAR0 + HARDDOOM_INTR_ENABLE);

  down(&drvdata->fifo_wait);

  enabled = enabled & ~HARDDOOM_INTR_PONG_ASYNC;
  iowrite32(enabled, drvdata->BAR0 + HARDDOOM_INTR_ENABLE);
}

static void fifo_send(struct doom_prv *drvdata, uint32_t cmd) {
  unsigned int fifo_free;
  fifo_free = ioread32(drvdata->BAR0 + HARDDOOM_FIFO_FREE);
  if (fifo_free < 1)
    fifo_wait(drvdata);
  iowrite32(cmd, drvdata->BAR0 + HARDDOOM_FIFO_SEND);
}

long doom_cmd(struct doom_prv *drvdata, uint32_t cmd) {
  down(&drvdata->fifo_queue);
  fifo_send(drvdata, cmd);

  drvdata->fifo_count = (drvdata->fifo_count + 1) % DOOM_PING_FREQ;
  if (drvdata->fifo_count == 0)
    fifo_send(drvdata, HARDDOOM_CMD_PING_ASYNC);

  up(&drvdata->fifo_queue);
  return 0;
}
