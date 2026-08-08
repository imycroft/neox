#include "pic.h"
#include "io.h"

#define PIC1_COMMAND   0x20
#define PIC1_DATA      0x21

#define PIC2_COMMAND   0xA0
#define PIC2_DATA      0xA1

#define ICW1_ICW4      0x01
#define ICW1_INIT      0x10

#define ICW4_8086      0x01

void pic_init(void)
{
    uint8_t master_mask = inb(PIC1_DATA);
    uint8_t slave_mask  = inb(PIC2_DATA);

    /* Start initialization */
    outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);

    /* New vector offsets */
    outb(PIC1_DATA, PIC_MASTER_OFFSET);
    outb(PIC2_DATA, PIC_SLAVE_OFFSET);

    /* Cascade configuration */
    outb(PIC1_DATA, 4);
    outb(PIC2_DATA, 2);

    /* 8086 mode */
    outb(PIC1_DATA, ICW4_8086);
    outb(PIC2_DATA, ICW4_8086);

    /* Restore interrupt masks */
    outb(PIC1_DATA, master_mask);
    outb(PIC2_DATA, slave_mask);
}

void pic_send_eoi(unsigned char irq)
{
    if (irq >= 8)
        outb(PIC2_COMMAND, 0x20);

    outb(PIC1_COMMAND, 0x20);
}

