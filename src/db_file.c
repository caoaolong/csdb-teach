#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include "csdb/db.h"
#include "lib/str.h"
#include "lib/log.h"
#include "lib/code.h"
#include "db_file.h"
#include "db_file_page.h"
#include "db_file_block.h"
#include "ds/sl_list.h"

static sl_list *db_list;

void db_file_init()
{
    db_list = sl_list_create();
    db_file_load();
}

static void db_file_create_threads(db_file_t *db_file, sl_list *list, int count, void *(*func)(void *))
{
    for (int i = 0; i < count; i++)
    {
        pthread_t *thread = malloc(sizeof(pthread_t));
        if (!thread)
        {
            log_error("Failed to allocate memory for thread\n");
            return;
        }
        if (pthread_create(thread, NULL, func, db_file) != 0)
        {
            log_error("Failed to create thread\n");
            free(thread);
            return;
        }
        sl_list_insert_back(list, sl_node_create((int64_t)thread));
    }
}

static char *read_value(db_file_t *db_file, data_ref_t *ref)
{
    db_file_page_t *page = db_file_page(db_file, ref->page);
    if (!page->data)
    {
        db_file_page_read_data(page);
    }
    return page->data + ref->offset;
}

static void *db_file_read_thread(void *args)
{
    db_file_t *db_file = (db_file_t *)args;
    sl_list *pages = db_file->pages;
    while (true)
    {
        sem_wait(&db_file->rsem);
        pthread_mutex_lock(&db_file->rlock);
        data_block_prepare_t *block = (data_block_prepare_t *)array_remove_front(db_file->rbuf);
        pthread_mutex_unlock(&db_file->rlock);

        data_ref_t ref;
        db_file_page_t *page = NULL;
        if (block->type == ROW_TABLE)
        {
            // 匹配查询
            page = db_file_find(db_file, block->name, block->type, &ref);
        }
        else if (block->type == ROW_COLUMN)
        {
            ref.page = block->next.page;
            ref.offset = block->next.offset;
            // 直接索引
            page = db_file_page(db_file, block->next.page);
        }
        if (!page)
        {
            log_error("row %s not found\n", block->name);
            free(page);
            return NULL;
        }
        db_schema_row_t *schema = (db_schema_row_t *)(page->data + ref.offset);
        block->schema = schema;
        block->size = sizeof(db_schema_row_t);
        // 读取注释
        if (schema->comment > 0)
        {
            ref_decode(schema->comment, &ref);
            db_file_page_t *page = db_file_page(db_file, ref.page);
            // 读取数据
            schema->comment = (uint64_t)read_value(db_file, &ref);
        }
        sem_post(&block->sem);
    }
}

static void *db_file_write_thread(void *args)
{
    pthread_t thread = pthread_self();
    log_info("write thread %lu started\n", (unsigned long)thread);
    db_file_t *db_file = (db_file_t *)args;
    while (true)
    {
        sem_wait(&db_file->wsem);
        pthread_mutex_lock(&db_file->wlock);
        data_block_list_t *list = (data_block_list_t *)array_remove_front(db_file->wbuf);
        pthread_mutex_unlock(&db_file->wlock);
        if (!list || !list->list)
        {
            log_error("No data block list to process\n");
            continue;
        }

        db_file_page_t *last_page = NULL;
        for (int i = 0; i < list->list->size; i++)
        {
            sl_node *node = (sl_node *)sl_list_remove_front(list->list);
            data_block_t *block = (data_block_t *)node->data;
            if (!block)
            {
                log_error("Failed to get data block from list\n");
                continue;
            }

            if (block->flags & BLOCK_COVER)
            {
                db_file_page_t *page = db_file_page(db_file, block->cover.page);
                db_file_page_cover_row(page, block->cover.offset, block->data, block->size);
            }
            else
            {
                // 写入数据块到文件
                db_file_page_t *page = db_file_alloc_page(db_file, block->type, CSDB_DB_PAGE_ROW_SIZE);
                if (!page)
                {
                    log_error("No available page to write data block\n");
                    continue;
                }
                if (last_page)
                {
                    // 非第一次写入，设置数据页链表
                    last_page->header.next = page->header.page;
                }
                else
                {
                    // 第一次写入，更新ref
                    list->ref.page = page->header.page;
                    list->ref.offset = page->header.size;
                }
                db_file_page_write_row(page, block->data, block->size);
                last_page = page;
            }
        }
        sem_post(&list->sem);
        array_remove_value(db_file->wbuf, (int64_t)list);
    }
}

static db_file_t *db_file_build(const char *filepath)
{
    db_file_t *db_file = (db_file_t *)malloc(sizeof(db_file_t));
    if (!db_file)
    {
        perror("Failed to allocate memory for db_file");
        return NULL;
    }
    db_file->filename = strdup(filepath);
    db_file->prc = db_file->pwc = 2;
    // 初始化文件
    db_file->fp = fopen(filepath, "r+");
    db_file->size = CSDB_DB_FILE_SIZE;
    db_file->pages = sl_list_create();
    // 初始化缓冲区
    db_file->rbuf = array_create();
    db_file->wbuf = array_create();
    pthread_mutex_init(&db_file->rlock, NULL);
    pthread_mutex_init(&db_file->wlock, NULL);
    // 初始化信号量
    sem_init(&db_file->rsem, 0, 0);
    sem_init(&db_file->wsem, 0, 0);
    // 初始化读写线程
    db_file->prl = sl_list_create();
    db_file->pwl = sl_list_create();
    db_file_create_threads(db_file, db_file->prl, db_file->prc, db_file_read_thread);
    db_file_create_threads(db_file, db_file->pwl, db_file->pwc, db_file_write_thread);
    return db_file;
}

void db_file_load()
{
    struct dirent *entry;
    DIR *dp = opendir(CSDB_DB_FILE_DATA);
    while ((entry = readdir(dp)) != NULL)
    {
        if (!strendswith(entry->d_name, CSDB_DB_FILE_EXTENSION))
            continue;
        if (entry->d_type != 8)
            continue;
        char filepath[512];
        memset(filepath, 0, sizeof(filepath));
        snprintf(filepath, sizeof(filepath), "%s/%s", CSDB_DB_FILE_DATA, (char *)entry->d_name);
        // 创建数据库指针
        db_file_t *db_file = db_file_build(filepath);
        // 检测是否是合法的数据库文件
        fseek(db_file->fp, 0, SEEK_END);
        long fsz = ftell(db_file->fp);
        if (fsz < CSDB_DB_FILE_SIZE)
        {
            fclose(db_file->fp);
            free(db_file->filename);
            free(db_file);
            return;
        }
        // 计算页数
        uint32_t page_count = fsz / CSDB_DB_FILE_PAGE_SIZE;
        // 加载数据页
        int total = db_file_page_load(db_file, page_count);
        sl_list_insert_back(db_list, sl_node_create((int64_t)db_file));
        log_info("Loaded database file %s with %d pages\n", db_file->filename, total);
    }
    closedir(dp);
}

int db_file_create(const char *dbname)
{
    char filepath[256];
    memset(filepath, 0, sizeof(filepath));
    sprintf(filepath, "%s/%s.%s", CSDB_DB_FILE_DATA, dbname, CSDB_DB_FILE_EXTENSION);
    // 检测数据库文件是否存在
    FILE *fp = fopen(filepath, "r+");
    if (fp)
    {
        log_error("database %s already exists\n", dbname);
        return -1;
    }
    // 以读写方式打开文件
    fp = fopen(filepath, "w+");
    if (!fp)
    {
        log_error("failed to create database file %s\n", filepath);
        return -1;
    }
    // 初始化数据库文件
    db_file_t *db_file = db_file_build(filepath);
    // 初始化文件页
    ftruncate(fileno(db_file->fp), CSDB_DB_FILE_SIZE);
    uint32_t page_count = db_file->size / CSDB_DB_FILE_PAGE_SIZE;
    for (uint32_t i = 0; i < page_count; i++)
    {
        db_file_page_create(db_file, i + 1);
    }
    fflush(db_file->fp);
    // 插入数据库文件链表
    sl_list_insert_back(db_list, sl_node_create((uint64_t)db_file));
    return 0;
}

db_file_t *db_file_open(const char *dbname)
{
    char filename[256];

    int i = db_list->size;
    for (int i = 0; i < db_list->size; i++)
    {
        sl_node *node = (sl_node *)sl_list_get(db_list, i);
        db_file_t *db_file = (db_file_t *)node->data;
        memset(filename, 0, sizeof(filename));
        sprintf(filename, "%s.%s", dbname, CSDB_DB_FILE_EXTENSION);
        if (strendswith(db_file->filename, filename))
        {
            return db_file;
        }
    }
}

data_block_prepare_t *db_file_read(db_file_t *db_file, const void *name, uint32_t page, uint16_t offset, uint16_t data_type)
{
    data_block_prepare_t *block = data_block_prepare();
    block->type = data_type;
    if (data_type == ROW_TABLE)
    {
        strcpy(block->name, name);
    }
    else if (data_type == ROW_COLUMN)
    {
        block->next.page = page;
        block->next.offset = offset;
    }
    // 提交任务
    pthread_mutex_lock(&db_file->rlock);
    array_insert_back(db_file->rbuf, (int64_t)block);
    pthread_mutex_unlock(&db_file->rlock);
    sem_post(&db_file->rsem);
    log_info("commit read task for %s\n", name);
    // 等待任务执行完毕
    clock_t begin = clock();
    sem_wait(&block->sem);
    clock_t end = clock();
    log_info("read task completed in %f seconds\n", (double)(end - begin) / CLOCKS_PER_SEC);
    return block;
}

void db_file_write_schema(db_file_t *db_file, db_schema_row_t *schema, bool commit)
{
    data_ref_t ref;
    // 写入注释
    if (schema->comment > 0)
    {
        char *comment = (char *)schema->comment;
        db_file_write(db_file, &ref, comment, strlen(comment) + 1, DB_FILE_VALUE, commit);
        schema->comment = ref_encode(&ref);
    }
    db_file_write(db_file, &ref, (char *)schema, sizeof(db_schema_row_t), DB_FILE_SCHEMA, commit);
    schema->self = ref_encode(&ref);
}

void db_file_link_schema(db_file_t *db_file, db_schema_row_t *schema, bool commit)
{
    db_file_cover(db_file, (char *)schema, sizeof(db_schema_row_t), DB_FILE_SCHEMA, commit);
}

void db_file_cover(db_file_t *db_file, char *data, size_t size, uint16_t type, bool commit)
{
    // 文件分块
    size_t remaining = size;
    data_block_list_t *list = data_block_list_create();
    data_block_create(list, data, size, type, BLOCK_COVER);
    // 提交写任务
    log_info("commit cover task with %d blocks\n", list->list->size);
    pthread_mutex_lock(&db_file->wlock);
    array_insert_back(db_file->wbuf, (int64_t)list);
    pthread_mutex_unlock(&db_file->wlock);
    sem_post(&db_file->wsem);
    // 等待任务执行完毕
    clock_t begin = clock();
    sem_wait(&list->sem);
    // 销毁数据块列表
    data_block_list_destroy(list);
    // 提交数据
    if (commit)
    {
        log_info("cover task committed\n");
        db_file_commit(db_file);
    }
    clock_t end = clock();
    log_info("cover task completed in %f seconds\n", (double)(end - begin) / CLOCKS_PER_SEC);
}

void db_file_write(db_file_t *db_file, data_ref_t *ref, char *data, size_t size, uint16_t type, bool commit)
{
    // 文件分块
    size_t remaining = size;
    data_block_list_t *list = data_block_list_create();
    data_block_create(list, data, size, type, BLOCK_DEFAULT);
    // 提交写任务
    log_info("commit write task with %d blocks\n", list->list->size);
    pthread_mutex_lock(&db_file->wlock);
    array_insert_back(db_file->wbuf, (int64_t)list);
    pthread_mutex_unlock(&db_file->wlock);
    sem_post(&db_file->wsem);
    // 等待任务执行完毕
    clock_t begin = clock();
    sem_wait(&list->sem);
    // 更新写入位置
    ref->page = list->ref.page;
    ref->offset = list->ref.offset;
    // 销毁数据块列表
    data_block_list_destroy(list);
    // 提交数据
    if (commit)
    {
        log_info("write task committed\n");
        db_file_commit(db_file);
    }
    clock_t end = clock();
    log_info("write task completed in %f seconds\n", (double)(end - begin) / CLOCKS_PER_SEC);
}

void db_file_close(db_file_t *db_file)
{
    if (db_file)
    {
        if (db_file->fp)
        {
            fclose(db_file->fp);
        }
        free(db_file->filename);
        free(db_file);
    }
    if (db_list->size == 0)
    {
        sl_list_destroy(db_list);
    }
}

void db_file_commit(db_file_t *db_file)
{
    for (int i = 0; i < db_file->pages->size; i++)
    {
        sl_node *node = (sl_node *)sl_list_get(db_file->pages, i);
        db_file_page_t *page = (db_file_page_t *)node->data;
        if (page->dirty)
        {
            db_file_page_commit(page);
        }
    }
}

db_file_page_t *db_file_alloc_page(db_file_t *db_file, uint8_t type, uint32_t size)
{
    for (int i = 0; i < db_file->pages->size; i++)
    {
        sl_node *node = (sl_node *)sl_list_get(db_file->pages, i);
        db_file_page_t *page = (db_file_page_t *)node->data;
        if (page->header.flags & type)
        {
            if (CSDB_DB_FILE_PAGE_SIZE - page->header.size >= size)
            {
                page->header.flags |= (DB_FILE_USED | type);
                return page;
            }
            else
            {
                continue;
            }
        }
        // 一个新的结构页
        if (!(page->header.flags & DB_FILE_USED))
        {
            page->header.flags = (DB_FILE_USED | type);
            return page;
        }
    }
    return NULL;
}

db_file_page_t *db_file_page(db_file_t *db_file, int index)
{
    for (int i = 0; i < db_file->pages->size; i++)
    {
        sl_node *node = (sl_node *)sl_list_get(db_file->pages, i);
        db_file_page_t *page = (db_file_page_t *)node->data;
        if (index == page->header.page)
        {
            return page;
        }
    }
    return NULL;
}

db_file_page_t *db_file_find(db_file_t *db_file, const char *name, uint16_t data_type, data_ref_t *ref)
{
    for (int i = 0; i < db_file->pages->size; i++)
    {
        sl_node *node = (sl_node *)sl_list_get(db_file->pages, i);
        db_file_page_t *page = (db_file_page_t *)node->data;

        if (!(page->header.flags & DB_FILE_USED))
            continue;

        if (page->header.flags & DB_FILE_SCHEMA)
        {
            int offset = db_file_page_find(page, name, data_type);
            if (offset < 0)
            {
                continue;
            }
            ref->page = page->header.page;
            ref->offset = offset;
            return page;
        }
    }
    return NULL;
}