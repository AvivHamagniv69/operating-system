#pragma once
#include "idt.h"

typedef enum Status {
    READY,
    RUNNING,
    DEAD,
} Status;

typedef struct Process {
    Status status;
    Regs* context;
    struct Process* next;
} Process;