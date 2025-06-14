#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "limine.h"
#include "apci.h"
#include "serial.h"
#include "util.h"

RsdpDescriptor* rsdp_v1 = NULL;
RsdpDescriptorV2* rsdp_v2 = NULL;

__attribute__((used, section(".limine_requests")))
static volatile struct limine_rsdp_request rsdp_request = {
    .id = LIMINE_RSDP_REQUEST,
    .revision = 3,
};

static bool validate_rsdp(uint8_t* rsdp, size_t size) {
    uint32_t sum = 0;
    for(int i = 0; i < size; i++) {
        sum += rsdp[i];
    }
    return (sum & 0xFF) == 0;
}

#define PIC_COMMAND_MASTER 0X20
#define PIC_DATA_MASTER    0X21
#define PIC_COMMAND_SLAVE  0XA0
#define PIC_DATA_SLAVE     0xA1

// ICW = Initialization Command Words
// ICW_1 a word that indicates a start of initialization sequence, it is the same for both the master and slave pic.
#define ICW_1 0X11

// are just the interrupt vector address value (IDT entries), 
// since the first 31 interrupts are used by the exceptions/reserved,
// we need to use entries above this value (remember that each pic has 8 different irqs that can handle.
#define ICW_2_M 0X20
#define ICW_2_S 0X28

// used to indicate if the pin has a slave or not. 
// (since the slave pic will be connected to one of the interrupt pins of the master we need to indicate which one is),
// or in case of a slave device the value will be its id.
// On x86 architectures the master irq pin connected to the slave is the second, this is why the value of ICW_M is 2
#define ICW_3_M 0X2
#define ICW_3_S 0X4

#define ICW_4 1

void disable_pic() {
    outb(PIC_COMMAND_MASTER, ICW_1);
    outb(PIC_COMMAND_SLAVE, ICW_1);
    outb(PIC_DATA_MASTER, ICW_2_M);
    outb(PIC_DATA_SLAVE, ICW_2_S);
    outb(PIC_DATA_MASTER, ICW_3_M);
    outb(PIC_DATA_SLAVE, ICW_3_S);
    outb(PIC_DATA_MASTER, ICW_4);
    outb(PIC_DATA_SLAVE, ICW_4);
    outb(PIC_DATA_MASTER, 0xFF);
    outb(PIC_DATA_SLAVE, 0xFF);
}

// revision should be 2 for v2, and 0 for v1
static void set_functions_for_revision(uint8_t revision) {

}

void rsdp_init(void) {
    struct limine_rsdp_response* response = rsdp_request.response;
    if(response == NULL) {
        serial_log("rsdp response is null, hang");
        hcf();
    }

    RsdpDescriptor* rsdp = (RsdpDescriptor*)response->address;
    if(rsdp->revision == 2) {
        rsdp_v2 = (RsdpDescriptorV2*)response->address;
        set_functions_for_revision(2);

        if(validate_rsdp((uint8_t*)rsdp_v2, sizeof(RsdpDescriptorV2)) == false) {
            serial_log("rsdp descriptor not valid, hang");
            hcf();
        }
    }
    else {
        rsdp_v1 = rsdp;
        set_functions_for_revision(0);

        if(validate_rsdp((uint8_t*)rsdp_v1, sizeof(RsdpDescriptor)) == false) {
            serial_log("rsdp descriptor not valid, hang");
            hcf();
        }
    }
}