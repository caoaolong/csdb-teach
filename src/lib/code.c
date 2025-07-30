#include "lib/code.h"

uint64_t ref_encode(data_ref_t *ref)
{
    uint64_t value = ref->page;
    value <<= 32;
    value |= ref->offset;
    return value;
}

void ref_decode(uint64_t index, data_ref_t *ref)
{
    ref->page = index >> 32;
    ref->offset = index & 0xFFFF;
}