#ifndef CSDB_DB_FILE_PAGE_H
#define CSDB_DB_FILE_PAGE_H

#include <stdint.h>
#include "db_file.h"

typedef struct db_file_page_header_s {
    // 数据页魔数
    char magic[4];
    // 当前页号
    uint32_t page;
    // 下一页号
    uint32_t next;
    // 当前页数据长度
    uint16_t size;
    // 保留字段
    uint16_t reserved;
} db_file_page_header_t;

struct db_file_page_s {
    db_file_t *db_file;
    // 文件页头信息
    db_file_page_header_t header;
};

void db_file_page_create(db_file_t *db_file, uint32_t page);

int db_file_page_load(db_file_t *db_file, uint32_t count);

int db_file_page_write(db_file_page_t *page, const void *data, size_t size);

int db_file_page_read(db_file_page_t *page, void *data, size_t size);

#endif // CSDB_DB_FILE_PAGE_H