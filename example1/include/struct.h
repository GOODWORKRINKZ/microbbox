#ifndef STRUCT_H
#define STRUCT_H

#include <cstdint>
#include <sys/types.h>

#define MAX_VALUES_COUNT 20
struct RSSIReading
{
    uint16_t frequency;
    uint8_t value;
};

#endif