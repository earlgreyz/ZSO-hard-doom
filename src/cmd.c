#include <linux/pci.h>
#include <linux/spinlock.h>

#include "../include/harddoom.h"

#include "cmd.h"

#define DOOM_FIFO_LENGTH 4096
#define DOOM_FIFO_SIZE   ((DOOM_FIFO_LENGTH + 1) * sizeof(uint32_t))
#define DOOM_PING_FREQ   (DOOM_FIFO_LENGTH / 8)

int cmd_init(struct doom_prv *drvdata) {
  drvdata->cmd = (uint32_t *) dma_alloc_coherent(drvdata->pci, DOOM_FIFO_SIZE, &drvdata->cmd_dma, GFP_KERNEL);
  if (drvdata->cmd == NULL)
    return -ENOMEM;
  drvdata->cmd[DOOM_FIFO_LENGTH] = HARDDOOM_CMD_JUMP(drvdata->cmd_dma);
  return 0;
}

void cmd_destroy(struct doom_prv *drvdata) {
  dma_free_coherent(drvdata->pci, DOOM_FIFO_SIZE, drvdata->cmd, drvdata->cmd_dma);
}

static bool cmd_can_insert(struct doom_prv *drvdata, uint8_t n) {
  uint32_t read_ptr, read_idx, next_idx;
  bool flip;

  next_idx = (drvdata->cmd_idx + n) % DOOM_FIFO_LENGTH;
  flip = next_idx < drvdata->cmd_idx + n;

  read_ptr = ioread32(drvdata->BAR0 + HARDDOOM_CMD_READ_PTR);
  read_idx = (read_ptr - (uint32_t) drvdata->cmd_dma) / sizeof(uint32_t);

  return ((read_idx == drvdata->cmd_idx)
      || (read_idx < drvdata->cmd_idx && flip && next_idx < read_idx)
      || (read_idx < drvdata->cmd_idx && !flip)
      || (read_idx > drvdata->cmd_idx && !flip && next_idx < read_idx));
}

void cmd_wait(struct doom_prv *drvdata, uint8_t n) {
  unsigned int enabled;

  iowrite32(HARDDOOM_INTR_PONG_ASYNC, drvdata->BAR0 + HARDDOOM_INTR);
  while (!cmd_can_insert(drvdata, n)) {
    enabled = ioread32(drvdata->BAR0 + HARDDOOM_INTR_ENABLE);
    enabled |= HARDDOOM_INTR_PONG_ASYNC;
    iowrite32(enabled, drvdata->BAR0 + HARDDOOM_INTR_ENABLE);

    down(&drvdata->fifo_wait);

    enabled = enabled & ~HARDDOOM_INTR_PONG_ASYNC;
    iowrite32(enabled, drvdata->BAR0 + HARDDOOM_INTR_ENABLE);
    iowrite32(HARDDOOM_INTR_PONG_ASYNC, drvdata->BAR0 + HARDDOOM_INTR);
  }

  drvdata->fifo_free = n;
}

static void cmd_send(struct doom_prv *drvdata, uint32_t cmd) {
  if (drvdata->fifo_free < 1) {
    printk(KERN_NOTICE "[cmd] cmd_send called without cmd_wait!");
    cmd_wait(drvdata, 1);
  }

  drvdata->cmd[drvdata->cmd_idx] = cmd;
  drvdata->cmd_idx = (drvdata->cmd_idx + 1) % DOOM_FIFO_LENGTH;
  drvdata->fifo_free--;
}

void cmd(struct doom_prv *drvdata, uint32_t cmd) {
  cmd_send(drvdata, cmd);

  drvdata->fifo_count = (drvdata->fifo_count + 1) % DOOM_PING_FREQ;
  if (drvdata->fifo_count == 0) {
    cmd_wait(drvdata, drvdata->fifo_free + 1);
    cmd_send(drvdata, HARDDOOM_CMD_PING_ASYNC);
  }
}

void cmd_commit(struct doom_prv *drvdata) {
  dma_addr_t addr = drvdata->cmd_dma + drvdata->cmd_idx * sizeof(uint32_t);
  iowrite32(addr, drvdata->BAR0 + HARDDOOM_CMD_WRITE_PTR);
}
