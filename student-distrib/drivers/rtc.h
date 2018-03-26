/* rtc.h - Defines rtc driver
 */

#ifndef _RTC_H
#define _RTC_H

#include "../types.h"
#include "../lib.h"
#include "../idt.h"

#define RTC_PORT1 0x70
#define RTC_PORT2 0x71
#define REGC 0x0C
#define REGA 0x8A

#define LOW_4_MASK 0xF0
#define SELECT_REGB 0x8B
#define SELECTBIT6 0x40
#define FREQ_2 15
#define FREQ_4 14
#define FREQ_8 13
#define FREQ_16 12
#define FREQ_32 11
#define FREQ_64 10
#define FREQ_128 9
#define FREQ_256 8
#define FREQ_512 7
#define FREQ_1024 6

volatile int rtc_interrupt_occurred;

/* handler for ticks */
void process_rtc();
/* open */
int32_t open_rtc(const uint8_t* filename);
/* read */
int32_t read_rtc(int32_t fd, void* buf, int32_t nbytes);
/* write */
int32_t write_rtc(int32_t fd, const void* buf, int32_t nbytes);
/* close */
int32_t close_rtc(int32_t fd);

int CP2_RTC_FLAG;

#endif /* _RTC_H */
