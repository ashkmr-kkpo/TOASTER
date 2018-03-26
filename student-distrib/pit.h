#ifndef _pit_H
#define _pit_H

#include "types.h"
#include "lib.h"
#include "drivers/terminal.h"

#define PIT_FREQ 1193180
#define DESIRED_FREQ 100
#define COMMAND_REG 0x43
#define DATAPORT0 0x40
#define IOCOMMAND 0x36 // channel 0, access mode: lobyte/hibyte, square wave generator, 16b binary


void pit_init(void);

#endif
