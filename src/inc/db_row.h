#ifndef CSDB_ROW_H
#define CSDB_ROW_H

#include <stdint.h>

enum {
    ROW_FILE,   // 文件
    ROW_TABLE,  // 数据表
    ROW_COLUMN, // 数据列
};

// 102bytes对齐
typedef struct db_schema_row_s {
    char name[64];          // 结构名称
    uint16_t data_type;     // 结构类型
    uint32_t start_page;    // 数据所在起始页
    uint64_t comment;       // 注释
    char unused[26];        // 保留空间
} db_schema_row_t;

db_schema_row_t *db_schema_new(const char *name, const char *comment, uint16_t subtype);

db_schema_row_t *db_schema_row_create(const char *name, uint16_t data_type, uint32_t page);

#endif // CSDB_ROW_H