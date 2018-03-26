/* terminal.h - Defines terminal keyboard driver
 */

#ifndef _TERMINAL_H
#define _TERMINAL_H

#include "../types.h"
#include "../x86_desc.h"
#include "../lib.h"
#include "../i8259.h"
#include "../idt.h"
#include "../syscalls.h"
#include "rtc.h"
#include "../paging.h"
//ESCAPE      0x01 
#define DELETE      0x0E 
#define ENTER       0x1C 
#define CTRL        0x1D 
#define SHIFT_L     0x2A 
#define SHIFT_R     0x36 
#define ALT         0x38 
#define SPACE       0x39
#define CAPSLOCK    0x3A

#define UNSHIFT_L   0xAA
#define UNSHIFT_R   0xB6
#define UNCTRL      0x9D
#define UNCHAR_L    0xA6
#define CHAR_L      0x26
#define UNALT       0xB8
#define KEYB_IRQ    1

#define LINE_WIDTH  80
#define BUFFER_W    128

/* holds the current keyboard buffer */
unsigned char term_buffer[3][128];
/* holds the current keyboard buffer location to add a character */
unsigned short term_loc[3];
// unsigned short read_loc;
char video_buffer[3][4000];

/* used to toggle corresponding key */
unsigned short ctrl_l;
unsigned short alt_l;
unsigned short shift;
unsigned short capslock;
/* flag to check if entered was recently pressed */
unsigned short entered[3];
/* current terminal : 0, 1, or 2 */
int current_terminal;

int terms_started[2];

/* initializes variables and clears screen */
void init_terminal(void);
/* processes scan_code of currently pressed key */
void process_key(unsigned char scan_code);
/* moves current keyboard buffer to the top */
void buff_to_top(void);

/* sys_read handlers for stdout and stdin */
int32_t stdout_read (int32_t fd, void* buf, int32_t nbytes);
int32_t stdin_read (int32_t fd, void* buf, int32_t nbytes);
/* sys_write handlers for stdout and stdin */
int32_t stdout_write (int32_t fd, const void* buf, int32_t nbytes);
int32_t stdin_write (int32_t fd, const void* buf, int32_t nbytes);
/* sys_open handlers for terminal (same for stdout/stdin) */
int32_t terminal_open (const uint8_t* filename);
/* sys_close handlers for terminal (same for stdout/stdin) */
int32_t terminal_close (int32_t fd);

#endif /* _TERMINAl_H */
