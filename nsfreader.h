#pragma once 

#include <stdint.h>

typedef struct nsfreader_s nsfreader_t;

struct nsfreader_s
{
    nsfreader_t* self;
    uint32_t(*read)(nsfreader_t* self, uint8_t* out, uint32_t offset, uint32_t length);
    uint32_t(*size)(nsfreader_t* self);
};

