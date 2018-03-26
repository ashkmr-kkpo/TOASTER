#include "pit.h"

/* pit_init
 *    INPUT: none
 * FUNCTION: sets the frequency of the PIT to 100 Hz for scheduling
 */
void
pit_init(void)
{
	int divisor = PIT_FREQ / DESIRED_FREQ;
	outb(IOCOMMAND, COMMAND_REG);
	outb(divisor & 0xFF, DATAPORT0); //0xFF = low byte masking
	outb(divisor >> 8, DATAPORT0); // >> 8 = high byte masking
}
