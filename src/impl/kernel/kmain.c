#include <stdint.h>
#include <stddef.h>
#include "print.h"

void kmain() {
    clear_screen();
    print_str("hello world", 11);
    return;
}