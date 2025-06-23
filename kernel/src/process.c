#include "process.h"
#include "mem.h"
#include "idt.h"

Process* head_process;
Process* current_process;
Process* final_process;

uint64_t process_new(char* name, void (*starting_address)(void* args), void* args) {
    static uint64_t cur_pid = 1;

    if(!name) {
        return 0;
    }

    Process* process = kmalloc(sizeof(process));
    // todo: fill name using strncpy
    process->pid = cur_pid;
    cur_pid++;
    process->status = READY;
    // todo: add context
    final_process->next = process;
    process->next = head_process;

    return process->pid;
}

// todo: implement ttl
Regs schedule(Regs* context) {
    current_process->context = *context;
    if(head_process == final_process) {
        return current_process->context;
    }

    // check and remove dead processes, do this before changing processes so that we dont switch to a dead one
    Process* p = head_process->next;
    Process* prev = head_process;
    while(p != head_process) {
        if(p->status == DEAD) {
            prev->next = p->next;
            kfree(p);
            p = prev->next;
        }
        else {
            current_process = current_process->next;
            return current_process->context;       
        }
    }
}