#ifndef HARDDOOM_CMD_H
#define HARDDOOM_CMD_H

#include "private.h"

int cmd_init(struct doom_prv *drvdata);
void cmd_destroy(struct doom_prv *drvdata);

void cmd(struct doom_prv *drvdata, uint32_t cmd);
void cmd_send(struct doom_prv *drvdata);

#endif
