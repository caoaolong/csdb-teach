#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "db_file_block.h"
#include "lib/code.h"

data_block_list_t *data_block_list_create()
{
    data_block_list_t *list = (data_block_list_t *)malloc(sizeof(data_block_list_t));
    if (!list)
    {
        perror("Failed to allocate memory for data block list");
        return NULL;
    }
    sem_init(&list->sem, 0, 0);
    list->list = sl_list_create();
    return list;
}

void data_block_list_destroy(data_block_list_t *db_list)
{
    if (db_list)
    {
        sem_destroy(&db_list->sem);
        sl_list_destroy(db_list->list);
        free(db_list);
    }
}

void data_block_create(data_block_list_t *list, char *data, size_t size, uint16_t type, uint8_t flags)
{
    uint16_t wsz = 0;
    while (size > 0)
    {
        wsz = size > DATA_BLOCK_SIZE ? DATA_BLOCK_SIZE : size;
        data_block_t *block = (data_block_t *)malloc(sizeof(data_block_t));
        block->size = wsz;
        block->type = type;
        block->flags = BLOCK_DEFAULT | flags;
        if (block->flags & BLOCK_COVER)
        {
            db_schema_row_t *row = (db_schema_row_t *)data;
            ref_decode(row->self, &block->cover);
        }
        memcpy(block->data, data, wsz);
        sl_list_insert_back(list->list, sl_node_create((int64_t)block));
        // 更新剩余字节数
        size -= wsz;
    }
}

data_block_prepare_t *data_block_prepare()
{
    data_block_prepare_t *block = (data_block_prepare_t *)malloc(sizeof(data_block_prepare_t));
    block->page = block->size = 0;
    block->data = NULL;
    sem_init(&block->sem, 0, 0);
    return block;
}