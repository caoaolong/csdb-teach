#ifndef INDEX_H
#define INDEX_H

#include <stddef.h>
#include <stdint.h>

typedef struct
{
    int64_t id;
    int64_t offset;
} idx_entry_t;

void derive_idx_path(const char *csv_path, char *out, size_t cap);

int build_index(const char *csv_path);

int idx_lookup(const char *csv_path, int64_t qid, int64_t *offset_out);

#endif
