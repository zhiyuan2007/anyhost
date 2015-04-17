#ifndef DOMAIN_WBLIST_HEADER_H
#define DOMAIN_WBLIST_HEADER_H
#include "dig_domain_rbtree.h"

typedef struct domain_wblist domain_wblist_t;

const char *
domain_wblist_get_result(int type);

domain_wblist_t *
domain_wblist_create();

void
domain_wblist_destroy(domain_wblist_t *domain_wblist);

void
domain_wblist_clear(domain_wblist_t  *domain_wblist);

int
domain_wblist_insert_from_text(domain_wblist_t *domain_wblist,
                               const char *domain_name);
int
domain_wblist_insert_from_wire(domain_wblist_t *domain_wblist,
                               const wire_name_t *wire_name);

int
domain_wblist_delete_form_text(domain_wblist_t *domain_wblist,
                               const char *domain_name);
int
domain_wblist_delete_from_wire(domain_wblist_t *domain_wblist,
                               wire_name_t *wire_name);

int
domain_wblist_find_from_text(domain_wblist_t *domain_wblist,
                             const char *domain_name);
int
domain_wblist_find_from_wire(domain_wblist_t *domain_wblist,
                             const wire_name_t *wire_name);
uint32_t
domain_wblist_get_count(domain_wblist_t *domain_wblist);

#endif
