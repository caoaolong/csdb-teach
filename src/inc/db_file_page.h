#ifndef CSDB_DB_FILE_PAGE_H
#define CSDB_DB_FILE_PAGE_H

#include <stdint.h>
#include <stdbool.h>
#include "db_file.h"
#include "db_row.h"

enum {
    DB_FILE_USED    = 0b00000001, // 页已使用
    DB_FILE_DATA    = 0b00000010, // 数据页
    DB_FILE_SCHEMA  = 0b00000100, // 结构页
    DB_FILE_VALUE   = 0b00001000, // 字符串值页
};

typedef struct db_file_page_header_s {
    // 数据页魔数
    char magic[4];
    // 当前页号
    uint32_t page;
    // 下一页号
    uint32_t next;
    // 当前页数据长度
    uint16_t size;
    // 页面属性
    uint8_t flags;
    // 保留字段
    uint8_t reserved;
} db_file_page_header_t;

struct db_file_page_s {
    // 文件描述符
    int fd;
    // 是否为脏页
    bool dirty;
    // 文件页头信息
    db_file_page_header_t header;
    // 数据
    char *data;
};

void db_file_page_create(db_file_t *db_file, uint32_t page);

int db_file_page_load(db_file_t *db_file, uint32_t count);

int db_file_page_write_header(db_file_page_t *page);

int db_file_page_read_header(db_file_page_t *page, uint32_t index);

int db_file_page_read_data(db_file_page_t *page);

int db_file_page_write_data(db_file_page_t *page, const void *data, size_t size);

char *db_file_page_read_row(db_file_page_t *page, int index);

int db_file_page_cover_row(db_file_page_t *page, uint16_t offset, const void *data, uint16_t size);

int db_file_page_write_row(db_file_page_t *page, const void *data, uint16_t size);

void db_file_page_commit(db_file_page_t *page);

int db_file_page_find(db_file_page_t *page, const char *name, uint16_t data_type);

#endif // CSDB_DB_FILE_PAGE_H