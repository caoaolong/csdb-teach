#ifndef CSDB_DB_FILE_BLOCK_H
#define CSDB_DB_FILE_BLOCK_H

#include <semaphore.h>
#include "csdb/db.h"
#include "db_file_page.h"
#include "ds/sl_list.h"

#define DATA_BLOCK_SIZE  (CSDB_DB_FILE_PAGE_SIZE - sizeof(db_file_page_header_t))

typedef struct data_block_s {
    uint16_t size; // 数据长度
    char data[DATA_BLOCK_SIZE]; // 数据块
} data_block_t;

typedef struct data_block_list_s {
    sem_t sem; // 信号量
    size_t bc; // 数据块数量
    sl_list *list; // 数据块列表
} data_block_list_t;

data_block_list_t *data_block_list_create();

void data_block_list_destroy(data_block_list_t *db_list);

data_block_t *data_block_create(char *data, size_t *remaining);

#endif // CSDB_DB_FILE_BLOCK_H