#ifndef CSDB_DB_FILE_H
#define CSDB_DB_FILE_H

#include <stdio.h>
#include "ds/sl_list.h"

typedef struct db_file_page_s db_file_page_t;

typedef struct db_file_s {
    char *filename;
    FILE *fp;
    int size;
    sl_list *pages;
} db_file_t;

void db_file_init();

void db_file_load();

int db_file_create(const char *filename);
db_file_t *db_file_open(const char *filename);
int db_file_read(db_file_t *db_file, void *buffer, size_t size);
int db_file_write(db_file_t *db_file, const void *buffer, size_t size);
void db_file_close(db_file_t *db_file);

#endif // CSDB_DB_FILE_H