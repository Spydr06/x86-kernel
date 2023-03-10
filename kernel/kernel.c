#include "dev/char/console.h"
#include "dev/char/serial.h"
#include "lib/log.h"
#include "sys/gdt.h"
#include "sys/idt.h"
#include "sys/irq.h"
#include "dev/pit.h"
#include "dev/pic.h"

#include <stdio.h>
#include <stdlib.h>

#if !defined(__i686__)
    #error "Expected x86 architecture"
#endif

#define X86_OK 0

#define init_irq_handler(i, handler) irq_register_handler((i), (handler)); klog(KLOG_INFO, "irq: handler set: " #handler "()")

int x86_pc_init(void) {
    gdt_install_flat();
    klog(KLOG_NONE, "GDT installed");

    init_idt();
    klog(KLOG_NONE, "IDT installed");

    init_pit(SYSTEM_TICKS_PER_SEC);
    klog(KLOG_NONE, "i8253 (PIT) initialized @%d hz", SYSTEM_TICKS_PER_SEC);

    init_pic();
    klog(KLOG_NONE, "i8259 (PIC) initialized");

    init_irq_handler(IRQ_PIT, sys_tick_handler);
    init_irq_handler(1, sys_key_handler);

    return X86_OK;
}

void kmain(void) {
    console_init();
    klog(KLOG_NONE, "kmain() at address: 0x%x", (unsigned int) kmain);

    serial_configure_baud_rate(SERIAL_COM1, 1);
    serial_configure_line(SERIAL_COM1);
    serial_write(SERIAL_COM1, "Serial port COM1 initialized" SERIAL_NEWLINE, 30);

    if(x86_pc_init() != X86_OK)
        goto failure;
    

    x86_enable_int();

    while(1) {}

failure:
    klog(KLOG_FAIL, "kmain@failure: label reached");
    abort();
}

void __kpanic(const char* file, const char* function, int line)
{
    console_color(FB_LT_RED, FB_BLACK);
    printf("%s:%s():%d: ", file, function, line);
    abort();
}
