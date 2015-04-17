#ifndef ZIP_LRU_LIST_HEADER_H
#define ZIP_LRU_LIST_HEADER_H
#include "zip_pkg_list.h"

typedef struct lru_list lru_list_t;

lru_list_t * 
lru_list_create();

void
lru_list_delete(lru_list_t *lru);

pkg_list_t *
lru_list_remove_tail_list(lru_list_t *lru);

void
lru_list_insert(lru_list_t *lru_list, pkg_list_t *list);

void
lru_list_revise(lru_list_t *lru_list, pkg_list_t *pkg_list);

uint32_t 
lru_list_get_count(lru_list_t *lru_list);

void
lru_list_print(const lru_list_t *lru_list);

#endif
