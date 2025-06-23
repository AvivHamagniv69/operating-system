#pragma once
#include "idt.h"

typedef enum Status {
    READY,
    RUNNING,
    DEAD,
} Status;

#define NAME_MAX_LEN 8

typedef struct Process Process;
typedef struct Process {
    uint64_t pid;
    Status status;
    Regs context;
    void* root_page_table;
    Process* next;
    char name[NAME_MAX_LEN]
};