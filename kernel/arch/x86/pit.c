#include "pit.h"
#include "io.h"

#define PIT_CHANNEL0      0x40
#define PIT_COMMAND       0x43

#define PIT_FREQUENCY     1193182

static volatile uint32_t pit_ticks = 0;
static uint32_t pit_frequency = 0;

void pit_handler(void)
{
    pit_ticks++;
}

uint32_t pit_get_ticks(void)
{
    return pit_ticks;
}

void pit_init(uint32_t frequency)
{
    uint16_t divisor;

    pit_frequency = frequency;
    divisor = PIT_FREQUENCY / frequency;

    outb(PIT_COMMAND, 0x36);

    outb(PIT_CHANNEL0, divisor & 0xFF);
    outb(PIT_CHANNEL0, divisor >> 8);
}

uint32_t pit_get_frequency(void)
{
    return pit_frequency;
}
