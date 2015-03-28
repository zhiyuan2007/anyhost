#include "zip_lru_list.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "log_utils.h"

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
lru_list_destroy(lru_list_t *lru)
{
	lru_node_t *node = lru->header;
	while(node) {
		lru->header = node->next;
		free(node);
		node = lru->header;
	}
    free(lru);    
}

void
lru_list_delete(lru_list_t *list, lru_node_t *node)
{
    ASSERT(list && node, "lru list is NULL");     

	if (node == list->header){
		if (list->header->next) {
			list->header->next->prev = NULL;
			list->header = list->header->next;
		} else {
			list->header = list->tail = NULL;
		}
	}
	else if (node == list->tail ){
		node->prev->next = NULL;
		list->tail = node->prev;
	} else {
		node->prev->next = node->next;
		node->next->prev = node->prev;
	}
	free(node);
    list->count -= 1;
}


int
lru_list_get_count(lru_list_t *lru_list)
{
    return lru_list->count;
}

void *
lru_list_remove_tail(lru_list_t *lru)
{
    ASSERT(lru, "lru list is NULL");     
    if (!lru->tail)
        return NULL;

    lru_node_t *unpopular_node = lru->tail;
	if (unpopular_node == NULL) {
		return NULL;
	}
    lru_node_t *prev_list = unpopular_node->prev;
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
	void *rtn_node = NULL;
	if (unpopular_node) {
		rtn_node = unpopular_node->data;
	    free(unpopular_node);
    }
    return rtn_node;
}

lru_node_t *lru_list_search(lru_list_t *lru_list, void *user_data)
{
    lru_node_t *node = lru_list->header;
	while(node) {
		if (node->data == user_data) {
			return node;
		}
	}
	return NULL;
}

lru_node_t *
lru_list_insert(lru_list_t *lru_list, void *user_data)
{
    ASSERT(lru_list && user_data,"lru list insert parameter is NULL");
	lru_node_t *node = malloc(sizeof(lru_node_t));
	if (node == NULL) {
		printf("not enough memory");
		return;
	}
    node->next = node->prev = NULL;
	node->data = user_data;

    if (!lru_list->header && !lru_list->tail)
    {
        lru_list->header = node;
        lru_list->tail = node;
    }
    else
    {
        ASSERT(lru_list->header && lru_list->tail, "lru has at least one node");
        node->next = lru_list->header;
        lru_list->header->prev = node;
        lru_list->header = node;
    }

	lru_list->count++;

	return node;
}

#define PRINT_NODE_NAME(prefix, node) do{\
    printf(#prefix"%x", node);\
 }while(0)

void 
lru_list_print(const lru_list_t *list)
{
    printf("[\n");
    lru_node_t *node = list->header;
    while (node != NULL)
    {
        //PRINT_NODE_NAME(+,node);
        node = node->next;
        printf("\n");
    }
    printf(" ]\n ");

    return;
}

void
lru_list_move_to_first(lru_list_t *lru_list, struct lru_node *node)
{
    ASSERT(lru_list && node, "NULL pointer when lru list delete");

    if (node == lru_list->header)
        return ;

    node->prev->next = node->next;
    if (node == lru_list->tail)
        lru_list->tail = node->prev;
    else
        node->next->prev = node->prev;
	node->prev = NULL;
	node->next = lru_list->header;
	lru_list->header->prev = node;
	lru_list->header = node;
    return;
}
