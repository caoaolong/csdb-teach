#include <stdlib.h>
#include <string.h>
#include <unistd.h>
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
    db_file_page->db_file = db_file;
    db_file_page_header_build(db_file_page, page);
    db_file_page_write(db_file_page, &db_file_page->header, sizeof(db_file_page_header_t));

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
        db_file_page->db_file = db_file;
        if (!db_file_page)
        {
            perror("Failed to allocate memory for db_file_page");
            return -1;
        }
        db_file_page_read(db_file_page, &db_file_page->header, sizeof(db_file_page_header_t));


        // Add the page to the file's page list
        sl_node *node = sl_node_create((int64_t)db_file_page);
        sl_list_insert_back(db_file->pages, node);

        total++;
    }
    return total;
}

int db_file_page_write(db_file_page_t *page, const void *data, size_t size)
{
    fseek(page->db_file->fp, (page->header.page - 1) * CSDB_DB_FILE_PAGE_SIZE, SEEK_SET);
    return fwrite(data, size, 1, page->db_file->fp);
}

int db_file_page_read(db_file_page_t *page, void *data, size_t size)
{
    fseek(page->db_file->fp, (page->header.page - 1) * CSDB_DB_FILE_PAGE_SIZE, SEEK_SET);
    return fread(data, size, 1, page->db_file->fp);
}