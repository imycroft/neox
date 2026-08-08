#include "irq.h"
#include "pic.h"
#include "printf.h"
#include "keyboard.h"
#include "pit.h"
#include "scheduler.h"
#include "test.h"

/*
 * Handle hardware IRQs raised by the PIC.
 * This function translates the CPU interrupt number into the matching
 * PIC IRQ number, dispatches to the device-specific handler, acknowledges
 * the interrupt, and then triggers scheduler work for timer ticks.
 */
void irq_handler(struct registers *regs)
{
    /* Convert the CPU interrupt vector number (32-47) into the PIC IRQ number (0-15). */
    uint32_t irq = regs->int_no - 32;

    /* Route the interrupt to the correct handler. */
    switch (irq)
    {
        case 0:
            /* Timer interrupt: update PIT state and count elapsed ticks. */
            pit_handler();
            test_ticks++;
            break;

        case 1:
            /* Keyboard interrupt: process pending keyboard input. */
            keyboard_handler();
            break;
    }

    /*
     * Acknowledge the interrupt controller before any scheduling decision.
     * scheduler_tick() may switch away from this thread and never return,
     * so the PIC must already consider the IRQ serviced.
     */
    pic_send_eoi(irq);

    /* Only the timer IRQ should drive preemption and scheduling decisions. */
    if (irq == 0)
        scheduler_tick();
}
