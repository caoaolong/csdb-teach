#ifndef CSDB_CODE_H
#define CSDB_CODE_H

#include <stdint.h>
#include "db_file_block.h"

uint64_t ref_encode(data_ref_t *ref);
void ref_decode(uint64_t index, data_ref_t *ref);

#endif