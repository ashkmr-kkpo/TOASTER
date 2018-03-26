/* idt.c -  Defines various handler functions managing exeptions, 
 * interrupts, and system calls
 */

#include "idt.h"




/* Exception handlers: - same for EXCEP0 - EXCEP15
       Input: None
    Function: Handle cooresponding exception
              (currently just prints and loops indefinitely)
*/
void EXCEP0()
{
    printf("Division by zero\n");
    excep_loop();
}
void EXCEP1()
{
    printf("Debugger\n");
    excep_loop();
}
void EXCEP2()
{
    printf("NMI\n");
    excep_loop();
}
void EXCEP3()
{
    printf("Breakpoint\n");
    excep_loop();
}
void EXCEP4()
{
    printf("Overflow\n");
    excep_loop();
}
void EXCEP5()
{
    printf("Bounds\n");
    excep_loop();
}
void EXCEP6()
{
    printf("Invalid Opcode\n");
    excep_loop();
}
void EXCEP7()
{
    printf("Coprocessor not available\n");
    excep_loop();
}
void EXCEP8()
{
    printf("Double fault\n");
    excep_loop();
}
void EXCEP9()
{
    printf("Coprocessor Segment Overrun (386 or earlier only)\n");
    excep_loop();
}
void EXCEPA()
{
    printf("Invalid Task State Segment\n");
    excep_loop();
}
void EXCEPB()
{
    printf("Segment not present\n");
    excep_loop();
}
void EXCEPC()
{
    printf("Stack Fault\n");
    excep_loop();
}
void EXCEPD()
{
    printf("General protection fault\n");
    excep_loop();
}
void EXCEPE()
{
    uint32_t pfa;
    asm("movl %%cr2, %%eax;"
        : "=a"(pfa)
        : // no input
        : "cc"
        );
    printf("Page fault at %d\n", pfa);
    excep_loop();
}
void EXCEPF()
{
    printf("reserved\n");
    excep_loop();
}
void EXCEP10()
{
    printf("Math Fault\n");
    excep_loop();
}
void EXCEP11()
{
    printf("Alignment Check\n");
    excep_loop();
}
void EXCEP12()
{
    printf("Machine Check\n");
    excep_loop();
}
void EXCEP13()
{
    printf("SIMD Floating-Point Exception\n");
    excep_loop();
}
void EXCEP14()
{
    printf("Virtualization Exception\n");
    excep_loop();
}
void EXCEP15()
{
    printf("Control Protection Exception\n");
    excep_loop();
}

void EXCEPIGNORE()
{
    printf("Unknown Interrupt\n");
    excep_loop();
}

/* excep_loop
       Input: none
    Function: mask interrupts and loop indefinitely
 */
void excep_loop()
{
    cli();
    while(1);
}

/* KEYBOARD_HANDLER
       Input: none
    Function: Handles keyboard interrupts
 */
void KEYBOARD_HANDLER()
{
    asm volatile("pushal;");
    cli();

    unsigned char scan_code = inb(KEYBOARD_PORT); // 0x60 = keyboard signal port
    process_key(scan_code);

    sti();
    send_eoi(KEYB_IRQ);
    asm volatile("popal; leave; iret;");
}

/* RTC_HANDLER
       Input: none
    Function: Handles rtc interrupts
 */
void RTC_HANDLER()
{
    asm volatile("pushal;");
    cli();

    process_rtc();

    sti();
    send_eoi(RTC_IRQ);
    asm volatile("popal; leave; iret;");
}

/* PIT_HANDLER
       Input: none
    Function: Handles PIT interrupts, also performs task scheduling
 */
void PIT_HANDLER()
{
    cli();
    asm volatile("pushal;");
    timer_ticks++;

    // mod 2 to get interrupts every 20 ms
    if(timer_ticks % 2 == 0){

        PCB * pcb = get_pcb_ptr();
        
        // ---------- IF FIRST 2 START SHELLS AND EXECUTE---------- //
        if(timer_ticks <= THIRD_QUANT){ // start next two shells
            if(timer_ticks == FIRST_QUANT)
                current_terminal = 1; 
            else if(timer_ticks == SECOND_QUANT)
                current_terminal = 2;
            // save esp and ebp
            asm volatile("movl %%esp, %0;" : "=r"(pcb->esp));
            asm volatile("movl %%ebp, %0;" : "=r"(pcb->ebp));
            //
            sti();
            send_eoi(PIT_IRQ);
            sys_execute((uint8_t*)"shell");
            asm volatile("popal; leave; iret;");
        }
        else {
            // ---------- CHECK IF PROCESSES TO SWITCH TO AND GET PID ---------- //
            PCB * next_pcb;
            int next = 0, prev_process = running_process;
            while(!next){
                running_process = (running_process+1)%MAX_NUM_PROCESSES;  // go to next process
                if(running_process == prev_process)
                    break;
                if(process_array[running_process]){
                    //check if ! a process with parent
                    next_pcb = (PCB*)(_8MB - _8KB * (running_process + 1) );
                    if(next_pcb->child == -1){ // check if has no children processes
                        next = 1;
                    }
                }
            }

            //printf(" %d ", running_process, next);
            // ---------- CONTEXT SWITCH IF NEXT ---------- //
            if(next) {

                asm volatile("movl %%esp, %0;" : "=r"(pcb->esp));
                asm volatile("movl %%ebp, %0;" : "=r"(pcb->ebp));

                program_paging(_8MB + ((running_process) * _4MB));      // change to next processes page
                tss.esp0 = _8MB - _8KB * (running_process);

                asm volatile("movl %0, %%esp;"
                             "movl %1, %%ebp;"
                             :
                             :"r"(next_pcb->esp), "r"(next_pcb->ebp)
                );
                int parent_terminal = process_term();
                int offset;
                if(parent_terminal == current_terminal)
                    offset = 0;
                else
                    offset = ((parent_terminal + 1) * KiB4);
                user_video_page_table[0].address = (VIDEO_MEM_ADDRESS + offset) >> 12; // shift by 12 to remove non-address bits
            }
        }
    }

    sti();
    send_eoi(PIT_IRQ);
    asm volatile("popal; leave; iret;");
}

/* init_idt
        INPUT: none
     FUNCTION: builds idt by calling local functions
 */
void init_idt() {
    // setup scheduling data
    running_process = 0; // 0 is first process
    timer_ticks = 0;
    // initialize idt
    setup_idt();
    idt_exceptions();
    set_trap(syscallhandle, (int)SYS_IDT); // 0x80 = location in idt for systemcall
    set_interrupt(KEYBOARD_HANDLER, (int)KEYBOARD_IDT); // 0x21 = location in idt for IRQ1
    set_interrupt(RTC_HANDLER, (int)RTC_IDT); // 0x28 = location in idt for IRQ8
    set_interrupt(PIT_HANDLER, (int)PIT_IDT);
    open_rtc(NULL);

    enable_irq(KEYB_IRQ);
    enable_irq(RTC_IRQ);
    enable_irq(PIT_IRQ);
}

/* setup_idt
        INPUT: none
     FUNCTION: set all values of IDT to the EXCEPIGNORE exception (null exception) 
 */
void setup_idt() {
    int i;
    for(i = 0; i <NUM_VEC; i++)
        set_interrupt(EXCEPIGNORE, i);
}

/* idt_exceptions
        INPUT: none
     FUNCTION: set all values of IDT to correctly handle exceptions 
 */
void idt_exceptions()   { 
    int i;
    void (*EXCEPTHANDLRPTR[NUM_EXCEP])() = { // 22 = number of exceptions
        &EXCEP0, &EXCEP1, &EXCEP2, &EXCEP3, &EXCEP4, 
        &EXCEP5, &EXCEP6, &EXCEP7, &EXCEP8, &EXCEP9,
        &EXCEPA, &EXCEPB, &EXCEPC, &EXCEPD, &EXCEPE, 
        &EXCEPF, &EXCEP10, &EXCEP11, &EXCEP12, &EXCEP13, 
        &EXCEP14, &EXCEP15};
    for(i = 0; i <NUM_EXCEP; i++) {
        set_interrupt(EXCEPTHANDLRPTR[i], i);
    }
}

/*  set_interrupt
        INPUT: HANDLER - address of handler function for cooresponding descriptor
               i - location in IDT to place descriptor
     FUNCTION: creates and place descriptor in IDT based of parameters
*/
void set_interrupt(void (*HANDLER), int i) {
    idt[i].seg_selector = KERNEL_CS;
    idt[i].reserved4 =  0x00;
    idt[i].reserved3 =  0;
    idt[i].reserved2 =  1;
    idt[i].reserved1 =  1;
    idt[i].size =       1;
    idt[i].reserved0 =  0;
    idt[i].dpl =        0; // 0 = unprivileged access
    idt[i].present =    1;
    idt[i].offset_31_16 =  ((int)HANDLER & FIRST16) >> 16; // mask and bit shift highest 2 bytes
    idt[i].offset_15_00 = (int)HANDLER & LAST16;  // mask lowest 2 bytes
}

/*  set_trap
        INPUT: HANDLER - address of handler function for cooresponding descriptor
               i - location in IDT to place descriptor
     FUNCTION: creates and place descriptor in IDT based of parameters in trap mode
*/
void set_trap(void (*HANDLER), int i) {
    idt[i].seg_selector = KERNEL_CS;
    idt[i].reserved4 =  0x00;
    idt[i].reserved3 =  1;
    idt[i].reserved2 =  1;
    idt[i].reserved1 =  1;
    idt[i].size =       1;
    idt[i].reserved0 =  0;
    idt[i].dpl =        3; // 3 = privileged access
    idt[i].present =    1;
    idt[i].offset_31_16 =  ((int)HANDLER & FIRST16) >> 16; // mask and bit shift highest 2 bytes
    idt[i].offset_15_00 = (int)HANDLER & LAST16;  // mask lowest 2 bytes
}

