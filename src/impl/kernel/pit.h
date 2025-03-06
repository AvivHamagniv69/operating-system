#pragma once
#include "util.h"

void init_pit(void);

void on_irq0(InterruptRegisters* regs);