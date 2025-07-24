#include <stdlib.h>
#include <string.h>
#include "db_file_block.h"

data_block_list_t *data_block_list_create()
{
    data_block_list_t *list = (data_block_list_t *)malloc(sizeof(data_block_list_t));
    if (!list) {
        perror("Failed to allocate memory for data block list");
        return NULL;
    }
    sem_init(&list->sem, 0, 0);
    list->bc = 0;
    list->list = sl_list_create();
    return list;
}

void data_block_list_destroy(data_block_list_t *db_list)
{
    if (db_list) {
        sem_destroy(&db_list->sem);
        sl_list_destroy(db_list->list);
        free(db_list);
    }
}

data_block_t *data_block_create(char *data, size_t *remaining)
{
    uint16_t wsz = *remaining;
    if (wsz > DATA_BLOCK_SIZE) {
        wsz = DATA_BLOCK_SIZE;
    }
    *remaining -= wsz;
    data_block_t *block = (data_block_t *)malloc(sizeof(data_block_t));
    block->size = wsz;
    memcpy(block->data, data, wsz);
    return block;
}

data_block_prepare_t *data_block_prepare() 
{
    data_block_prepare_t *block = (data_block_prepare_t *)malloc(sizeof(data_block_prepare_t));
    block->page = block->size = 0;
    block->data = NULL;
    sem_init(&block->sem, 0, 0);
    return block;
}