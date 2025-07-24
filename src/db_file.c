#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include "csdb/db.h"
#include "lib/str.h"
#include "lib/log.h"
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
        sl_node *node = pages->head;
        // 计算数据大小
        size_t size = 0;
        db_file_page_t *page = db_file_page(db_file, block->page);
        while (page && page->header.flags & DB_FILE_USED)
        {
            size += page->header.size;
            // 读取下一页
            page = db_file_page(db_file, page->header.next);
        }
        // 读取数据
        block->data = malloc(size);
        page = db_file_page(db_file, block->page);
        while (page && page->header.flags & DB_FILE_USED)
        {
            int pgsz = db_file_page_read_data(page);
            if (pgsz < 0)
            {
                log_error("load data from page %d failed\n", page->header.page);
            }
            memcpy(block->data + block->size, page->data, pgsz);
            block->size += pgsz;
            // 读取下一页
            page = db_file_page(db_file, page->header.next);
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
        if (!list)
        {
            log_error("No data block list to process\n");
            continue;
        }

        db_file_page_t *last_page = NULL;
        for (int i = 0; i < list->bc; i++)
        {
            sl_node *node = (sl_node *)sl_list_remove_front(list->list);
            data_block_t *block = (data_block_t *)node->data;
            if (!block)
            {
                log_error("Failed to get data block from list\n");
                continue;
            }
            // 写入数据块到文件
            db_file_page_t *page = db_file_alloc_page(db_file);
            if (!page)
            {
                log_error("No available page to write data block\n");
                continue;
            }
            page->header.size = block->size;
            page->header.flags |= DB_FILE_USED; // 标记为已使用
            // 设置数据页链表
            if (last_page)
            {
                last_page->header.next = page->header.page;
                db_file_page_write_header(last_page);
            }
            db_file_page_write_header(page);
            db_file_page_write_data(page, block->data, block->size);
            last_page = page;
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

data_block_prepare_t *db_file_read(db_file_t *db_file, uint32_t page)
{
    if (page < 1 || page > db_file->pages->size) {
        log_error("page index %d out of range[%d, %d]", page, 1, db_file->pages->size);
        return NULL;
    }

    data_block_prepare_t *block = data_block_prepare();
    block->page = page;
    // 提交任务
    pthread_mutex_lock(&db_file->rlock);
    array_insert_back(db_file->rbuf, (int64_t)block);
    pthread_mutex_unlock(&db_file->rlock);
    sem_post(&db_file->rsem);
    log_info("commit read task from page %d\n", page);
    // 等待任务执行完毕
    clock_t begin = clock();
    sem_wait(&block->sem);
    clock_t end = clock();
    log_info("read task completed in %f seconds\n", (double)(end - begin) / CLOCKS_PER_SEC);
    return block;
}

void db_file_write(db_file_t *db_file, char *data, size_t size, bool commit)
{
    // 文件分块
    size_t remaining = size;
    data_block_list_t *list = data_block_list_create();
    do
    {
        data_block_t *block = data_block_create(data, &remaining);
        sl_list_insert_back(list->list, sl_node_create((int64_t)block));
        list->bc++;
    } while (remaining > 0);
    // 提交写任务
    log_info("commit write task with %d blocks\n", list->list->size);
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

db_file_page_t *db_file_alloc_page(db_file_t *db_file)
{
    for (int i = 0; i < db_file->pages->size; i++)
    {
        sl_node *node = (sl_node *)sl_list_get(db_file->pages, i);
        db_file_page_t *page = (db_file_page_t *)node->data;
        if (!page->dirty && !(page->header.flags & DB_FILE_USED))
        {
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