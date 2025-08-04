#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "csdb/db.h"
#include "db_file_page.h"
#include "ds/sl_list.h"

static void db_file_page_header_build(db_file_page_t *page, uint32_t index)
{
    page->header.page = index;
    page->header.next = 0;
    page->header.size = 0;     // Initialize size to 0
    page->header.reserved = 0; // Reserved field
    // Set magic number for identification
    memcpy(page->header.magic, CSDB_DB_FILE_MAGIC, sizeof(page->header.magic));
}

void db_file_page_create(db_file_t *db_file, uint32_t page)
{
    db_file_page_t *db_file_page = (db_file_page_t *)malloc(sizeof(db_file_page_t));
    if (!db_file_page)
    {
        perror("Failed to allocate memory for db_file_page");
        return;
    }
    db_file_page->fd = dup(fileno(db_file->fp));
    db_file_page_header_build(db_file_page, page);
    db_file_page_write_header(db_file_page);

    // Add the page to the file's page list
    sl_node *node = sl_node_create((int64_t)db_file_page);
    sl_list_insert_back(db_file->pages, node);
}

int db_file_page_load(db_file_t *db_file, uint32_t count)
{
    int total = 0;
    for (uint32_t i = 0; i < count; i++)
    {
        db_file_page_t *db_file_page = (db_file_page_t *)malloc(sizeof(db_file_page_t));
        db_file_page->fd = fileno(db_file->fp);
        if (!db_file_page)
        {
            perror("Failed to allocate memory for db_file_page");
            return -1;
        }
        db_file_page_read_header(db_file_page, i);

        // Add the page to the file's page list
        sl_node *node = sl_node_create((int64_t)db_file_page);
        sl_list_insert_back(db_file->pages, node);

        total++;
    }
    return total;
}

int db_file_page_write_header(db_file_page_t *page)
{
    size_t size = sizeof(db_file_page_header_t);
    lseek(page->fd, (page->header.page - 1) * CSDB_DB_FILE_PAGE_SIZE, SEEK_SET);
    int r = write(page->fd, &page->header, size);
    if (r != size)
    {
        perror("Failed to write page header");
        return -1;
    } else {
        page->dirty = true; // Mark the page as dirty after writing
        return r;
    }
}

int db_file_page_read_header(db_file_page_t *page, uint32_t index)
{
    lseek(page->fd, index * CSDB_DB_FILE_PAGE_SIZE, SEEK_SET);
    return read(page->fd, &page->header, sizeof(db_file_page_header_t));
}

char *db_file_page_read_row(db_file_page_t *page, int index)
{
    int offset = page->header.size + sizeof(db_file_page_header_t) + index * CSDB_DB_PAGE_ROW_SIZE;
    char *data = malloc(CSDB_DB_PAGE_ROW_SIZE);
    if (page->data) {
        memcpy(data, page->data + offset, CSDB_DB_PAGE_ROW_SIZE);
        return data;
    }
    lseek(page->fd, (page->header.page - 1) * CSDB_DB_FILE_PAGE_SIZE + offset, SEEK_SET);
    read(page->fd, data, CSDB_DB_PAGE_ROW_SIZE);
    return data;
}

int db_file_page_read_data(db_file_page_t *page)
{
    if (page->data) {
        return page->header.size;
    }

    size_t size = page->header.size;
    page->data = (char *)malloc(size);
    if (!page->data)
    {
        perror("Failed to allocate memory for page data");
        return -1;
    }
    lseek(page->fd, (page->header.page - 1) * CSDB_DB_FILE_PAGE_SIZE + sizeof(db_file_page_header_t), SEEK_SET);
    if (read(page->fd, page->data, size) < 0)
    {
        free(page->data);
        perror("Failed to read page data");
        return -1;
    }
    return size;
}

int db_file_page_cover_row(db_file_page_t *page, uint16_t offset, const void *data, uint16_t size)
{
    lseek(page->fd, (page->header.page - 1) * CSDB_DB_FILE_PAGE_SIZE + sizeof(db_file_page_header_t) + offset, SEEK_SET);
    int nbytes = write(page->fd, data, size);
    if (nbytes < size) {
        perror("write page row failed");
        return -1;
    }
    return 0;
}

int db_file_page_write_row(db_file_page_t *page, const void *data, uint16_t size)
{
    // 写入数据
    int offset = page->header.size + sizeof(db_file_page_header_t);
    lseek(page->fd, (page->header.page - 1) * CSDB_DB_FILE_PAGE_SIZE + offset, SEEK_SET);
    int nbytes = write(page->fd, data, size);
    if (nbytes < size) {
        perror("write page row failed");
        return -1;
    }
    // 更新头部数据
    page->header.size += size;
    lseek(page->fd, (page->header.page - 1) * CSDB_DB_FILE_PAGE_SIZE, SEEK_SET);
    return write(page->fd, &page->header, sizeof(db_file_page_header_t));
}

int db_file_page_write_data(db_file_page_t *page, const void *data, size_t size)
{
    lseek(page->fd, (page->header.page - 1) * CSDB_DB_FILE_PAGE_SIZE + sizeof(db_file_page_header_t), SEEK_SET);
    int r = write(page->fd, data, size);
    if (r != size)
    {
        perror("Failed to write page header");
        return -1;
    } else {
        page->dirty = true; // Mark the page as dirty after writing
        return r;
    }
}

void db_file_page_commit(db_file_page_t *page)
{
    fsync(page->fd); // Ensure all data is written to disk
    page->dirty = false; // Mark the page as clean after committing
}

int db_file_page_find(db_file_page_t *page, const char *name)
{
    // 读取本页数据
    db_file_page_read_data(page);
    // 查询
    int offset = 0;
    while (offset < page->header.size) {
        char *row_name = page->data + offset;
        if (!strcmp(row_name, name)) {
            return offset;
        }
        offset += CSDB_DB_PAGE_ROW_SIZE;
    }
    return 0;
}