#pragma once

#define PIC_MASTER_OFFSET 32
#define PIC_SLAVE_OFFSET  40

void pic_init(void);
void pic_send_eoi(unsigned char irq);
