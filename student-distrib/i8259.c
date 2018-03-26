/* i8259.c - Functions to interact with the 8259 interrupt controller
 * vim:ts=4 noexpandtab
 */

#include "i8259.h"
/* <send_eoi>
 *    INPUT: <none>
 * FUNCTION:  Initialize the 8259 PIC */

void
i8259_init(void)
{
	outb(ICW1, MASTER_8259_PORT);                      //INITIALIZE MASTER PIC
	outb(ICW2_MASTER, MASTER_8259_PORT + 1);
	outb(ICW3_MASTER, MASTER_8259_PORT + 1);            // ICW3: tell Master PIC that there is a slave PIC at IRQ2
	outb(ICW4, MASTER_8259_PORT + 1);

	outb(ICW1, SLAVE_8259_PORT);                       //INITIALIZE SLAVE PIC
	outb(ICW2_SLAVE, SLAVE_8259_PORT + 1);
	outb(ICW3_SLAVE, SLAVE_8259_PORT + 1);              // ICW3: tell Slave PIC its cascade identity 
	outb(ICW4, SLAVE_8259_PORT + 1);

	outb(BITMASK, MASTER_8259_PORT+1);                 // restore saved masks
	outb(BITMASK, SLAVE_8259_PORT+1);
    enable_irq(SLAVE_IRQ);

}
/* <enable_irq>
 *    INPUT: <irq num to enable>
 * FUNCTION:  Enable (unmask) the specified IRQ */

void
enable_irq(uint32_t irq_num)
{
	uint16_t port;
   	uint8_t value;
 	if(irq_num >OUT_OF_SLAVE_IRQ_BOUND || irq_num <0)         // SLAVES ARE FROM 8-15
 		return;
    if(irq_num < MASTER_IRQS) {
        port = MASTER_8259_PORT + 1;
    } else {
        port = SLAVE_8259_PORT + 1;
        irq_num -= 8;
    }
    value = ~(inb(port));                               //UNMASK ENABLED IRQ BIT 
    value=  value | (1 << irq_num);
    value= ~value;
    outb(value, port);        
}
/* <disable_irq>
 *    INPUT: < irq num to disable>
 * FUNCTION:   Disable (mask) the specified IRQ */


void
disable_irq(uint32_t irq_num)
{
	uint16_t port;
    uint8_t value;
 	if(irq_num >OUT_OF_SLAVE_IRQ_BOUND || irq_num <0)
 		return;
    if(irq_num < MASTER_IRQS) {
        port = MASTER_8259_PORT + 1;
    } else {
        port = SLAVE_8259_PORT + 1;
        irq_num -= 8;
    }
    value = ~(inb(port));
    value = value & ~(1 << irq_num);        //MASK ENABLED IRQ BIT 
    value= ~value;
    outb(value, port);  
}
/* <send_eoi>
 *    INPUT: <irq num that;s interrupt ended>
 * FUNCTION:  Send end-of-interrupt signal for the specified IRQ 
 */
/**/
extern void
send_eoi(uint32_t irq_num)
{
    asm volatile("pushal;");
	if(irq_num >= MASTER_IRQS){
        outb(EOI | (irq_num-8), SLAVE_8259_PORT);                   // IF IRQ IN SLAVE YOU NEED TO SEND EOI TO MASTER AND SLAVE
        outb(EOI + 2, MASTER_8259_PORT);
 	} else {
	    outb(EOI | irq_num, MASTER_8259_PORT);
    }
    asm volatile("popal;");
}

