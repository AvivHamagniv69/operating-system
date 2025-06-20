#include "idt.h"
#include "serial.h"
#include "util.h"
#include "process.h"

/* This will keep track of how many ticks that the system
*  has been running for */
int timer_ticks = 0;

void timer_phase(int hz) {
    int divisor = 1193180 / hz;       /* Calculate our divisor */
    outb(0x43, 0x36);             /* Set our command byte 0x36 */
    outb(0x40, divisor & 0xFF);   /* Set low byte of divisor */
    outb(0x40, divisor >> 8);     /* Set high byte of divisor */
}

/* Handles the timer. In this case, it's very simple: We
*  increment the 'timer_ticks' variable every time the
*  timer fires. By default, the timer fires 18.222 times
*  per second. Why 18.222Hz? Some engineer at IBM must've
*  been smoking something funky */
static void timer_handler(Regs* r) {
    /* Increment our 'tick count' */
    timer_ticks++;
    

    serial_log("timer ticked\n");
}

/* Sets up the system clock by installing the timer handler
*  into IRQ0 */
void pit_init() {
    /* Installs 'timer_handler' to IRQ0 */
    irq_install_handler(0, timer_handler);
}