#include "process.h"

Process* head_process;
Process* current_process;

void schedule() {
    current_process = current_process->next;
}