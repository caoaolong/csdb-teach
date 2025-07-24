#ifndef CSDB_DB_FILE_H
#define CSDB_DB_FILE_H

#include <stdio.h>
#include <pthread.h>
#include <stdbool.h>
#include <semaphore.h>
#include "ds/sl_list.h"
#include "ds/array.h"

typedef struct db_file_page_s db_file_page_t;
typedef struct data_block_prepare_s data_block_prepare_t;

typedef struct db_file_s {
    // 数据库文件路径
    char *filename;
    // 文件指针
    FILE *fp;
    // 文件大小
    int size;
    // 默认读写线程数
    int prc, pwc;
    // 读写线程链表
    sl_list *prl, *pwl;
    // 读写缓冲区
    array *rbuf, *wbuf;
    // 读写锁
    pthread_mutex_t rlock, wlock;
    // 读写信号量
    sem_t rsem, wsem;
    // 文件页链表
    sl_list *pages;
} db_file_t;

void db_file_init();
void db_file_load();

int db_file_create(const char *filename);
db_file_t *db_file_open(const char *dbname);
data_block_prepare_t *db_file_read(db_file_t *db_file, uint32_t page);
void db_file_write(db_file_t *db_file, char *data, size_t size, bool commit);
void db_file_close(db_file_t *db_file);
void db_file_commit(db_file_t *db_file);

db_file_page_t *db_file_alloc_page(db_file_t *db_file);
db_file_page_t *db_file_page(db_file_t *db_file, int index);

#endif // CSDB_DB_FILE_H