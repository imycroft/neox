#include "irq.h"
#include "pic.h"
#include "printf.h"
#include "keyboard.h"
#include "pit.h"
#include "scheduler.h"
void irq_handler(struct registers *regs)
{
    uint32_t irq = regs->int_no - 32;

    switch (irq)
    {
        case 0:
            pit_handler();
            break;

        case 1:
            keyboard_handler();
            break;
    }

    /*
     * Acknowledge the interrupt controller before any
     * scheduling decision. scheduler_tick() may context
     * switch away from this thread, suspending this call
     * indefinitely; the PIC must already consider IRQ0
     * serviced or the timer stops firing entirely and no
     * further preemption can occur.
     */
    pic_send_eoi(irq);

    if (irq == 0)
        scheduler_tick();
}
