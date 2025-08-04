#ifndef CSDB_ROW_H
#define CSDB_ROW_H

#include <stdint.h>
#include "db_file.h"

enum {
    ROW_FILE,   // 文件
    ROW_TABLE,  // 数据表
    ROW_COLUMN, // 数据列
};


// 102bytes对齐
struct __attribute__((packed)) db_schema_row_s {
    char name[64];          // (64 bytes) 结构名称
    uint16_t data_type;     //  (2 bytes) 结构类型
    uint32_t start_page;    //  (4 bytes) 数据所在起始页
    uint64_t self;          //  (8 bytes) 当前位置指针
    uint64_t comment;       //  (8 bytes) 注释指针
    uint64_t next;          //  (8 bytes) 链表指针
    char unused[8];         //  (8 bytes) 保留空间
};

db_schema_row_t *db_schema_new(const char *name, const char *comment, uint16_t data_type);

db_schema_row_t *db_schema_load_schema(db_file_t *db_file, const char *name, uint16_t data_type);

void db_schema_link(db_schema_row_t *self, db_schema_row_t *next);

#endif // CSDB_ROW_H