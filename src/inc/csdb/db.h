#ifndef CSDB_DB_H
#define CSDB_DB_H

// 初始化数据库文件大小
#define CSDB_DB_FILE_DATA       "./data"
// 数据库文件拓展名
#define CSDB_DB_FILE_EXTENSION  "csdb"
// 文件页魔数
#define CSDB_DB_FILE_MAGIC      "CSDB"
// 数据库文件初始化大小
#define CSDB_DB_FILE_SIZE       0x10000
// 数据库文件页大小
#define CSDB_DB_FILE_PAGE_SIZE  0x1000

#endif // CSDB_DB_H