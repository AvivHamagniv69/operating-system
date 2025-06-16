#include "process.h"
#include "mem.h"
#include "idt.h"

Process* head_process;
Process* current_process;

Regs* schedule(Regs* context) {
    current_process->context = context;
    current_process = current_process->next;
    return current_process->context;
}