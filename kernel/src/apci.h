#pragma once

typedef struct RsdpDescriptor {
    char signature[8];
    uint8_t checksum;
    char oemid[6];
    uint8_t revision;
    uint32_t rsdt_address;
} RsdpDescriptor __attribute__ ((packed));

typedef struct RsdpDescriptorV2 {
    char signature[8];
    uint8_t checksum;
    char oemid[6];
    uint8_t revision;
    uint32_t rsdt_address;

    uint32_t length;
    uint64_t xsdt_address;
    uint8_t extended_checksum;
    uint8_t reserved[3];
} RsdpDescriptorV2 __attribute__ ((packed));