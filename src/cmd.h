#ifndef HARDDOOM_CMD_H
#define HARDDOOM_CMD_H

#include "private.h"

int cmd_init(struct doom_prv *drvdata);
void cmd_destroy(struct doom_prv *drvdata);

/**
 * Blocks the execution until at least @param n commands can be inserted
 * to the device queue with the `cmd` function.
 **/
int __must_check cmd_wait(struct doom_prv *drvdata, uint8_t n);

/**
 * Inserts cmd at the end of the queue.
 * A `cmd` call should always be preceded with `cmd_wait` and succeeded with a
 * `cmd_send`. To achieve better performence it is recommended to use wait and
 * send functions once per block of the commands.
 **/
int __must_check cmd(struct doom_prv *drvdata, uint32_t cmd);

/**
 * Commits the commands queue to the device, moving WRITE_PTR to the most
 * recent command.
 **/
void cmd_commit(struct doom_prv *drvdata);

#endif
