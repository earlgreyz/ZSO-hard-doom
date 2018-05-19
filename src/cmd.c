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

static bool is_fifo_free(struct doom_prv *drvdata) {
  uint32_t read_ptr = ioread32(drvdata->BAR0 + HARDDOOM_CMD_READ_PTR);
  uint32_t read_idx = (read_ptr - (uint32_t) drvdata->cmd_dma) / sizeof(uint32_t);
  return (drvdata->cmd_idx + 1) % DOOM_FIFO_LENGTH != read_idx;
}

static void fifo_wait(struct doom_prv *drvdata) {
  unsigned int enabled;

  iowrite32(HARDDOOM_INTR_PONG_ASYNC, drvdata->BAR0 + HARDDOOM_INTR);

  while (!is_fifo_free(drvdata)) {
    enabled = ioread32(drvdata->BAR0 + HARDDOOM_INTR_ENABLE);
    enabled |= HARDDOOM_INTR_PONG_ASYNC;
    iowrite32(enabled, drvdata->BAR0 + HARDDOOM_INTR_ENABLE);

    down(&drvdata->fifo_wait);

    enabled = enabled & ~HARDDOOM_INTR_PONG_ASYNC;
    iowrite32(enabled, drvdata->BAR0 + HARDDOOM_INTR_ENABLE);

    iowrite32(HARDDOOM_INTR_PONG_ASYNC, drvdata->BAR0 + HARDDOOM_INTR);
  }
}

static void fifo_send(struct doom_prv *drvdata, uint32_t cmd) {
  if (!is_fifo_free(drvdata))
    fifo_wait(drvdata);

  drvdata->cmd[drvdata->cmd_idx] = cmd;
  drvdata->cmd_idx = (drvdata->cmd_idx + 1) % DOOM_FIFO_LENGTH;
}

void cmd(struct doom_prv *drvdata, uint32_t cmd) {
  down(&drvdata->fifo_queue);
  fifo_send(drvdata, cmd);

  drvdata->fifo_count = (drvdata->fifo_count + 1) % DOOM_PING_FREQ;
  if (drvdata->fifo_count == 0)
    fifo_send(drvdata, HARDDOOM_CMD_PING_ASYNC);

  up(&drvdata->fifo_queue);
}

void cmd_send(struct doom_prv *drvdata) {
  dma_addr_t addr = drvdata->cmd_dma + drvdata->cmd_idx * sizeof(uint32_t);
  down(&drvdata->fifo_queue);
  iowrite32(addr, drvdata->BAR0 + HARDDOOM_CMD_WRITE_PTR);
  up(&drvdata->fifo_queue);
}
