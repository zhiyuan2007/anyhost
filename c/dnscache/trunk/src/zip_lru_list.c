#include "zip_lru_list.h"
#include <stdio.h>
#include <string.h>
#include "zip_utils.h"

struct lru_list
{
    struct pkg_list *header;
    struct pkg_list *tail;
    uint32_t count;
};

lru_list_t * 
lru_list_create()
{
    lru_list_t *lru = malloc(sizeof(lru_list_t));
    if (!lru)
        return NULL;
    lru->header = NULL;
    lru->tail = NULL;
    lru->count = 0;

    return lru;
}

void
lru_list_delete(lru_list_t *lru)
{
    free(lru);    
}

uint32_t 
lru_list_get_count(lru_list_t *lru_list)
{
    return lru_list->count;
}

pkg_list_t *
lru_list_remove_tail_list(lru_list_t *lru)
{
    ASSERT(lru, "lru list is NULL");     
    if (!lru->tail)
        return NULL;

    pkg_list_t *unpopular_list = lru->tail;
    pkg_list_t *prev_list = unpopular_list->prev;
    if (prev_list)
    {
        lru->tail = prev_list;
        prev_list->next = NULL;
    }
    else
    {
        lru->header = lru->tail = NULL;
    }
    lru->count -= 1;
    return unpopular_list;
}


void
lru_list_insert(lru_list_t *lru_list, pkg_list_t *list)
{
    ASSERT(lru_list && list ,"lru list insert parameter is NULL");
    list->next = list->prev = NULL;

    if (!lru_list->header && !lru_list->tail)
    {
        lru_list->header = list;
        lru_list->tail = list;
    }
    else
    {
        ASSERT(lru_list->header && lru_list->tail, "lru has at least one node");
        list->next = lru_list->header;
        lru_list->header->prev = list;
        lru_list->header = list;
    }

    lru_list->count += 1;
}

#define PRINT_NODE_NAME(prefix, node) do{\
     char name_buf[512];\
    buffer_t buffer;\
    buffer_create_from(&buffer, name_buf, 512);\
    wire_name_to_text(pkg_list_get_owner(node), &buffer); \
    printf(#prefix"%s", name_buf);\
 }while(0)

void 
lru_list_print(const lru_list_t *list)
{
    printf("[\n");
    pkg_list_t *node = list->header;
    while (node != NULL)
    {
        PRINT_NODE_NAME(+,node);
        node = node->next;
        printf("\n");
    }
    printf(" ]\n ");

    return;
}

void
lru_list_revise(lru_list_t *lru_list, pkg_list_t *pkg_list)
{
    ASSERT(lru_list && pkg_list, "NULL pointer when lru list delete");

    if (pkg_list == lru_list->header)
        return ;

    pkg_list->prev->next = pkg_list->next;
    if (pkg_list == lru_list->tail)
        lru_list->tail = pkg_list->prev;
    else
        pkg_list->next->prev = pkg_list->prev;
    

    lru_list->count -= 1;
    lru_list_insert(lru_list, pkg_list);
    return;
}
