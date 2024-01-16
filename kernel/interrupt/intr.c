#include "intr.h"

#include "../syscall/syscall_kernel.h"
#include "../time/pit.h"
#include "common/cpu/contrl.h"
#include "err.h"
#include "idt.h"

#define PIC1 0x20 /* IO base address for master PIC */
#define PIC2 0xA0 /* IO base address for slave PIC */
#define PIC1_COMMAND PIC1
#define PIC1_DATA (PIC1 + 1)
#define PIC2_COMMAND PIC2
#define PIC2_DATA (PIC2 + 1)

/* reinitialize the PIC controllers, giving them specified vector offsets
   rather than 8h and 70h, as configured by default */

#define ICW1_ICW4 0x01      /* Indicates that ICW4 will be present */
#define ICW1_SINGLE 0x02    /* Single (cascade) mode */
#define ICW1_INTERVAL4 0x04 /* Call address interval 4 (8) */
#define ICW1_LEVEL 0x08     /* Level triggered (edge) mode */
#define ICW1_INIT 0x10      /* Initialization - required! */

#define ICW4_8086 0x01       /* 8086/88 (MCS-80/85) mode */
#define ICW4_AUTO 0x02       /* Auto (normal) EOI */
#define ICW4_BUF_SLAVE 0x08  /* Buffered mode/slave */
#define ICW4_BUF_MASTER 0x0C /* Buffered mode/master */
#define ICW4_SFNM 0x10       /* Special fully nested (not) */

/**
 * @see https://wiki.osdev.org/8259_PIC
 */
void pic_init() {
    uint8_t a1, a2;

    a1 = inb(PIC1_DATA);  // save masks
    a2 = inb(PIC2_DATA);

    outb(
        PIC1_COMMAND,
        ICW1_INIT |
            ICW1_ICW4);  // starts the initialization sequence (in cascade mode)
    io_wait();
    outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    outb(PIC1_DATA, 0x20);  // ICW2: Master PIC vector offset
    io_wait();
    outb(PIC2_DATA, 0x28);  // ICW2: Slave PIC vector offset
    io_wait();
    outb(PIC1_DATA, 4);  // ICW3: tell Master PIC that there is a slave PIC at
                         // IRQ2 (0000 0100)
    io_wait();
    outb(PIC2_DATA,
         2);  // ICW3: tell Slave PIC its cascade identity (0000 0010)
    io_wait();

    outb(PIC1_DATA,
         ICW4_8086);  // ICW4: have the PICs use 8086 mode (and not 8080 mode)
    io_wait();
    outb(PIC2_DATA, ICW4_8086);
    io_wait();

    outb(PIC1_DATA, 0);  // restore saved masks.
    outb(PIC2_DATA, 0);
}

extern intr_desc idt[INT_DESC_CNT];
void init_int() {
    init_intr();
    init_exception();
    lidt((uint32_t)idt, sizeof(idt));
    pic_init();
    pit_init();
    init_syscall();
    sti();
}

intr_handler intr_handlers[INT_DESC_CNT];

void register_intr_handler(int intr_no, intr_handler handler) {
    intr_handlers[intr_no] = handler;
}