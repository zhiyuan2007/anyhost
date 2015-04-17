#include "zip_pkg_list.h"
#include <stdio.h>
#include <string.h>
#include "zip_utils.h"


struct pkg_data
{
    void *data;
    uint16_t data_len;
    uint16_t type;
    struct pkg_data *next;
};

pkg_data_t *
pkg_data_create_from(const char *raw_data, uint16_t data_len, uint16_t type)
{
    pkg_data_t *pkg = malloc(sizeof(pkg_data_t));
    if (!pkg)
        return NULL;
    pkg->data = malloc(data_len);
    if (!pkg->data)
    {
        free (pkg);
        return NULL;
    }
    memcpy(pkg->data, raw_data, data_len);
    pkg->data_len = data_len;
    pkg->type = type;
    pkg->next = NULL;

    return pkg;
}

uint32_t
pkg_data_mem_size(pkg_data_t *pkg)
{
    return sizeof(pkg_data_t) + pkg->data_len;
}

void *
pkg_data_get_data(pkg_data_t *pkt)
{
    return pkt->data;
}

uint16_t 
pkg_data_get_type(pkg_data_t *pkt)
{
    return pkt->type;
}
uint16_t
pkg_data_get_len(pkg_data_t *pkt)
{   
    return pkt->data_len;
}

void pkg_data_free(pkg_data_t *pkt)
{
    free(pkt->data);
    free(pkt);
}

pkg_list_t *
pkg_list_create()
{
    pkg_list_t *list = malloc(sizeof(pkg_list_t));
    if (list)
    {
        list->header = NULL;
        list->count = 0;
        list->prev = NULL;
        list->next = NULL;
        list->owner = NULL;
        list->mem_size = sizeof(pkg_list_t);
    }

    return list;
}

void
pkg_list_destroy(pkg_list_t *list)
{
    ASSERT(list, "pkg list is NULL");
    pkg_data_t *node = list->header;
    while (node)
    {
        pkg_data_t *tmp = node->next;
        pkg_data_free(node);
        node = tmp;
    }
    if (list->owner)
        wire_name_delete(list->owner);
    free(list);
}

uint32_t 
pkg_list_get_count(pkg_list_t *pkg_list)
{
    return pkg_list->count;
}

const wire_name_t*
pkg_list_get_owner(pkg_list_t *list)
{
    return list->owner;
}

void
pkg_list_set_owner(pkg_list_t *list, wire_name_t *owner)
{
    list->owner = owner;
}

uint32_t
pkg_list_mem_size(pkg_list_t *list)
{
    return list->mem_size;
}

void
pkg_list_insert(pkg_list_t *list, pkg_data_t *pkg)
{
    ASSERT(list && pkg, "pkg list is NULL");

    pkg->next = list->header;
    list->header = pkg;
    list->count += 1;
    list->mem_size += pkg_data_mem_size(pkg);
}

void
pkg_list_delete(pkg_list_t *list, pkg_data_t *pkg)
{
    pkg_data_t *prev_node = NULL;
    pkg_data_t *node = list->header;
    while(node)
    {
        if (node == pkg)
            break;
        prev_node = node;
        node = node->next;    
    }
    if (node)
    {
        if (!prev_node)
            list->header = node->next;
        else
            prev_node->next = node->next;
        list->count -= 1;
        list->mem_size -= pkg_data_mem_size(node);
        pkg_data_free(node);
    }
}

pkg_data_t *
pkg_list_search(pkg_list_t *list, uint8_t query_type)
{
    ASSERT(list, "pkg list is NULL");

    pkg_data_t *node = list->header;
    while(node)
    {
        if (node->type == query_type)
            break;
        node = node->next;    
    }
    return node;
}
