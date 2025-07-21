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
#include "ds/sl_list.h"

static sl_list *db_list;

void db_file_init()
{
    db_list = sl_list_create();
    db_file_load();
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

        db_file_t *db_file = (db_file_t *)malloc(sizeof(db_file_t));
        if (!db_file)
        {
            perror("Failed to allocate memory for db_file");
            return;
        }
        db_file->filename = strdup(entry->d_name);
        // 初始化文件
        db_file->fp = fopen(filepath, "r+");
        db_file->size = CSDB_DB_FILE_SIZE;
        fseek(db_file->fp, 0, SEEK_END);
       long fsz = ftell(db_file->fp);
        // 初始化文件页
        db_file->pages = sl_list_create();
        // 检测是否是合法的数据库文件
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
    db_file_t *db_file = (db_file_t *)malloc(sizeof(db_file_t));
    db_file->fp = fp;
    db_file->filename = strdup(dbname);
    db_file->size = CSDB_DB_FILE_SIZE;
    db_file->pages = sl_list_create();
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

int db_file_read(db_file_t *db_file, void *buffer, size_t size)
{
    size_t bytes_read = 0;
    while (bytes_read < size)
    {
    }
}

int db_file_write(db_file_t *db_file, const void *buffer, size_t size)
{
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