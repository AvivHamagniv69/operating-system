#pragma once
#include "idt.h"

typedef enum Status {
    READY,
    RUNNING,
    DEAD,
} Status;

typedef struct Process {
    Status status;
    regs* context;
    struct Process* next;
} Process;