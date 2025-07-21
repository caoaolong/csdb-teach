#include "ds/sl_list.h"
#include <stdlib.h>
#include <stdio.h>

sl_list *sl_list_create()
{
    sl_list *list = malloc(sizeof(sl_list));
    list->head = list->tail = NULL;
    list->size = 0;
    return list;
}

sl_node *sl_node_create(int64_t data)
{
    sl_node *node = malloc(sizeof(sl_node));
    node->data = data;
    node->next = NULL;
    return node;
}

static void sl_list_insert_onlyone(sl_list *list, sl_node *node)
{
    list->head = list->tail = node;
    list->size++;
}

void sl_list_insert_back(sl_list *list, sl_node *node)
{
    if (!list || !node)
        return;

    if (list->size == 0)
    {
        sl_list_insert_onlyone(list, node);
        return;
    }

    list->tail->next = node;
    list->tail = node;
    list->size++;
}

void sl_list_insert_front(sl_list *list, sl_node *node)
{
    if (!list || !node)
        return;

    if (list->size == 0)
    {
        sl_list_insert_onlyone(list, node);
        return;
    }

    node->next = list->head;
    list->head = node;
    list->size++;
}

void sl_list_insert(sl_list *list, sl_node *node, uint32_t index)
{
    if (!list || !node || index < 0 || index >= list->size)
        return;

    if (index == 0)
    {
        sl_list_insert_front(list, node);
        return;
    }

    if (index == list->size - 1)
    {
        sl_list_insert_back(list, node);
        return;
    }

    if (list->size == 0)
    {
        sl_list_insert_onlyone(list, node);
        return;
    }

    sl_node *pn = list->head;
    while (index--)
    {
        pn = pn->next;
    }
    sl_node *tpn = pn->next;
    pn->next = node;
    node->next = tpn;
    list->size++;
}

sl_node *sl_list_get(sl_list *list, uint32_t index)
{
    if (!list || index < 0 || index >= list->size)
        return NULL;

    sl_node *pn = list->head;
    while (index--)
    {
        pn = pn->next;
    }
    return pn;
}

static sl_node *sl_list_remove_onlyone(sl_list *list)
{
    sl_node *pn = list->head;
    list->head = list->tail = NULL;
    list->size = 0;
    return pn;
}

sl_node *sl_list_remove_front(sl_list *list)
{
    if (!list)
        return NULL;

    if (list->size == 1)
    {
        return sl_list_remove_onlyone(list);
    }

    sl_node *tpn = list->head;
    list->head = list->head->next;
    list->size--;
    return tpn;
}

sl_node *sl_list_remove_back(sl_list *list)
{
    if (!list)
        return NULL;

    if (list->size == 1)
    {
        return sl_list_remove_onlyone(list);
    }

    sl_node *spn = list->head, *fpn = list->head->next;
    while (fpn->next)
    {
        spn = spn->next;
        fpn = fpn->next;
    }
    spn->next = NULL;
    list->tail = spn;
    list->size--;
    return fpn;
}

sl_node *sl_list_remove(sl_list *list, uint32_t index)
{
    if (!list || index < 0 || index >= list->size)
        return NULL;

    if (index == 0)
    {
        return sl_list_remove_front(list);
    }

    if (index == list->size - 1)
    {
        return sl_list_remove_back(list);
    }

    if (list->size == 1)
    {
        return sl_list_remove_onlyone(list);
    }

    sl_node *spn = list->head, *fpn = list->head->next;
    while (index-- > 1)
    {
        spn = spn->next;
        fpn = fpn->next;
    }
    spn->next = fpn->next;
    fpn->next = NULL;
    list->size--;
    return fpn;
}

void sl_list_destroy(sl_list *list)
{
    if (!list || list->size == 0)
        return;

    sl_node *pn = list->head;
    while (pn)
    {
        sl_node *tpn = pn->next;
        free(pn);
        pn = tpn;
    }
    free(list);
}
