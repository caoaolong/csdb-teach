#ifndef CSDB_DB_FILE_BLOCK_H
#define CSDB_DB_FILE_BLOCK_H

#include <semaphore.h>
#include "csdb/db.h"
#include "db_file_page.h"
#include "ds/sl_list.h"

enum
{
    BLOCK_DEFAULT = 0x00, // 是否为默认任务
    BLOCK_COVER = 0x01,   // 是否为覆盖型任务
};

#define DATA_BLOCK_SIZE (CSDB_DB_FILE_PAGE_SIZE - sizeof(db_file_page_header_t))

struct data_ref_s
{
    uint32_t page;
    uint16_t offset;
};

typedef struct data_block_s
{
    uint16_t size;              // 数据长度
    uint16_t type;              // 数据类型
    data_ref_t cover;           // 覆盖的位置(BLOCK_COVER时必传)
    char data[DATA_BLOCK_SIZE]; // 数据块
    uint8_t flags;              // 任务标志
} data_block_t;

struct data_block_prepare_s
{
    sem_t sem;     // 信号量
    uint32_t page; // 起始页号
    size_t size;   // 实际读取长度
    char *data;    // 数据
};

typedef struct data_block_list_s
{
    sem_t sem;      // 信号量
    data_ref_t ref; // 写入位置
    sl_list *list;  // 数据块列表
} data_block_list_t;

data_block_list_t *data_block_list_create();

void data_block_list_destroy(data_block_list_t *db_list);

void data_block_create(data_block_list_t *list, char *data, size_t size, uint16_t type, uint8_t flags);

data_block_prepare_t *data_block_prepare();

#endif // CSDB_DB_FILE_BLOCK_H