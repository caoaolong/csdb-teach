#ifndef INDEX_H
#define INDEX_H

#include <stddef.h>
#include <stdint.h>

#define IDX_MAGIC "CSDBIDX1"
#define IDX_MAGIC_LEN 8

typedef struct
{
    int64_t id;
    int64_t offset;
} idx_entry_t;

typedef struct
{
    char magic[IDX_MAGIC_LEN];
    int64_t csv_size;
    int64_t csv_mtime;
    uint64_t count;
} idx_header_t;

void derive_idx_path(const char *csv_path, char *out, size_t cap);

int build_index(const char *csv_path);

int idx_lookup(const char *csv_path, int64_t qid, int64_t *offset_out);

#endif
