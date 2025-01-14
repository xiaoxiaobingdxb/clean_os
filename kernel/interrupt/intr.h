#ifndef INTERRUPT_INTR_H
#define INTERRUPT_INTR_H
#include "idt.h"

void init_int();

void register_intr_handler(int intr_no, intr_handler handler);

void intr_enable(int intr_no);
void intr_disable(int intr_no);
#endif