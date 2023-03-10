#include "irq.h"
#include "../dev/char/console.h"
#include "../dev/pic.h"
#include "../dev/char/serial.h"

#include <stdio.h>
#include <stddef.h>

static InterruptHandler_T irq_routines[16] = {0};

void irq_register_handler(int irq, InterruptHandler_T handler)
{
    if(handler == NULL || irq < 0 || irq > 15)
        return;
    irq_routines[irq] = handler;
}

void irq_unregister_handler(int irq)
{
    if(irq < 0 || irq > 15)
        return;
    irq_routines[irq] = NULL;
}

void handle_platform_irq(IFrame_T *frame)
{
    uint32_t irq = frame->vector - 32;
    InterruptHandler_T handler = irq_routines[irq];
    if(handler)
        handler(frame);

    if(irq == IRQ_PIT)
        return;

    pic_send_EOI(irq);
}

void sys_tick_handler(IFrame_T* frame)
{
    pic_send_EOI(IRQ_PIT);
}
