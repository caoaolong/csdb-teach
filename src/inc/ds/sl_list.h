#ifndef DAA_SL_LIST_H
#define DAA_SL_LIST_H

#include <stddef.h>
#include <stdint.h>

typedef struct sl_node_t {
    int64_t data;
    struct sl_node_t *next;
} sl_node;

typedef struct sl_list_t {
    sl_node *head;
    sl_node *tail;
    uint32_t size;
} sl_list;

// 创建单向链表
sl_list *sl_list_create();

// 创建单向链表节点
sl_node *sl_node_create(int64_t data);

// 在单向链表的尾部添加节点
void sl_list_insert_back(sl_list *list, sl_node *node);

// 在单向链表的头部添加节点
void sl_list_insert_front(sl_list *list, sl_node *node);

// 在单向链表中插入节点
void sl_list_insert(sl_list *list, sl_node *node, uint32_t index);

// 获取单向链表的节点
sl_node *sl_list_get(sl_list *list, uint32_t index);

// 删除单向链表头部节点
sl_node *sl_list_remove_front(sl_list *list);

// 删除单向链表尾部节点
sl_node *sl_list_remove_back(sl_list *list);

// 删除单向链表的节点
sl_node *sl_list_remove(sl_list *list, uint32_t index);

// 释放单向链表
void sl_list_destroy(sl_list *list);

#endif