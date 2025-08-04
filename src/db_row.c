#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "db_row.h"
#include "lib/code.h"

db_schema_row_t *db_schema_new(const char *name, const char *comment, uint16_t data_type)
{
    db_schema_row_t *row = malloc(sizeof(db_schema_row_t));
    memset(row, 0, sizeof(db_schema_row_t));
    strcpy(row->name, name);
    row->data_type = data_type;
    row->comment = (uint64_t)comment;
    return row;
}

db_schema_row_t *db_schema_load_table(db_file_t *db_file, const char *name)
{
    data_block_prepare_t *block = db_file_read(db_file, name, 0, 0, ROW_TABLE);
    return block->schema;
}

db_schema_row_t *db_schema_load_column(db_file_t *db_file, uint32_t page, uint16_t offset)
{
    data_block_prepare_t *block = db_file_read(db_file, NULL, page, offset, ROW_COLUMN);
    return block->schema;
}

void db_schema_link(db_schema_row_t *self, db_schema_row_t *next)
{
    data_ref_t ref;
    ref_decode(next->self, &ref);
    self->next = ref_encode(&ref);
}