#include <stdint.h>
#include <stddef.h>
#include "print.h"
#include "util.h"
#include "gdt.h"

void kmain() {
    init_gdt();

    clear_screen();
    print_str("hello world");
    return;
}