#include "irq.h"
#include "pic.h"
#include "printf.h"
#include "keyboard.h"
#include "pit.h"
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

    pic_send_eoi(irq);
}
