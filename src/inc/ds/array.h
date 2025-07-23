#ifndef DS_ARRAY_H
#define DS_ARRAY_H

#include <stddef.h>
#include <stdint.h>

typedef struct array_t {
    int64_t *data;   // 数据
    size_t size;     // 数量
    size_t capacity; // 容量
} array;

// 创建数组
array *array_create();

// 加载数组
array *array_load(void *data, size_t size, size_t capacity);

// 从头部插入
void array_insert_front(array *array, int64_t data);

// 从尾部插入
void array_insert_back(array *array, int64_t data);

// 从中间插入
void array_insert(array *array, int index, int64_t data);

// 获取节点
int64_t array_node(const array *array, int index);

// 从头部删除
int64_t array_remove_front(array *array);

// 从尾部删除
int64_t array_remove_back(array *array);

// 从中间删除
int64_t array_remove(array *array, int index);

// 删除指定元素
int64_t array_remove_value(array *array, int64_t value);

// 释放数组
void array_destroy(array *array);

#endif
