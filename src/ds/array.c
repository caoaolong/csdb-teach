#include "csdb/db.h"
#include "ds/array.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <math.h>

array *array_create()
{
    array *array = malloc(sizeof(array));
    array->capacity = CSDB_DB_FILE_PAGE_SIZE;
    array->size = 0;
    array->data = malloc(sizeof(int64_t) * array->capacity);
    return array;
}

array *array_load(void *data, size_t size, size_t capacity)
{
    array *array = malloc(sizeof(array));
    if (!array) {
        perror("Failed to allocate memory for array");
        return NULL;
    }
    array->capacity = capacity;
    array->size = size;
    array->data = (uint64_t *)data;
    return array;
}

static bool array_is_full(array *array) 
{
    return array->size >= array->capacity;
}

static void array_expand(array *array)
{
    array->capacity *= 2;
    int64_t *data = realloc(array->data, sizeof(int64_t) * array->capacity);
    if (data) {
        array->data = data;
    } else {
        array->capacity /= 2;
    }
}

void array_insert_front(array *array, int64_t data)
{
    if (!array || !array->data) return;

    if (array_is_full(array))
        array_expand(array);

    array_insert(array, 0, data);
}

void array_insert_back(array *array, int64_t data)
{
    if (!array || !array->data) return;

    if (array_is_full(array))
        array_expand(array);

    array->data[array->size] = data;
    array->size++;
}

void array_insert(array *array, int index, int64_t data)
{
    if (!array || !array->data || index < 0 || index >= array->size) return;

    if (array_is_full(array))
        array_expand(array);

    for (int i = array->size - 1; i >= index; i--) {
        array->data[i + 1] = array->data[i];
    }

    array->data[index] = data;
    array->size++;
}

int64_t array_node(const array *array, int index)
{
    if (!array || !array->data) return -1;

    return array->data[index];
}

int64_t array_remove_front(array *array)
{
    if (!array || !array->data) return -1;

    if (array->size == 0) return -1;

    return array_remove(array, 0);
}

int64_t array_remove_back(array *array)
{
    if (!array || !array->data) return -1;

    if (array->size == 0) return -1;

    int64_t data = array->data[array->size - 1];
    array->size--;
    return data;
}

int64_t array_remove(array *array, int index)
{
    if (!array || !array->data || index < 0 || index >= array->size) return -1;

    if (array->size == 0) return -1;

    int64_t data = array->data[index];
    for (int i = index + 1; i <= array->size; i++) {
        array->data[i - 1] = array->data[i];
    }
    array->size--;
    return data;
}

int64_t array_remove_value(array *array, int64_t value)
{
    if (!array || !array->data) return -1;

    for (int i = 0; i < array->size; i++) {
        if (array->data[i] == value) {
            return array_remove(array, i);
        }
    }
    return -1; // Value not found
}

void array_destroy(array *arr)
{
    if (!arr) return;

    if (!arr->data) {
        free(arr);
        return;
    }

    free(arr->data);
    free(arr);
}
