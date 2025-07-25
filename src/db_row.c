#include <stdlib.h>
#include <string.h>
#include "db_row.h"

db_schema_row_t *db_schema_row_create(const char *name, uint16_t data_type, uint32_t page)
{
    db_schema_row_t *row = malloc(sizeof(db_schema_row_t));
    memset(row, 0, sizeof(db_schema_row_t));
    if (!row) {
        perror("Failed to allocate memory for db_schema_row");
        return NULL;
    }
    strcpy(row->name, name);
    row->data_type = data_type;
    row->start_page = page;
    return row;
}