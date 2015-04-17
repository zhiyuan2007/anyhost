#ifndef IP_WBLIST_HEADER_H
#define IP_WBLIST_HEADER_H
#include "zip_radix_tree.h"

typedef struct ip_wblist ip_wblist_t;

ip_wblist_t *
ip_wblist_create();

void
ip_wblist_destroy(ip_wblist_t *ip_wblist);

int
ip_wblist_insert_from_int(ip_wblist_t *ip_wblist, uint32_t key, uint32_t mask);

int
ip_wblist_insert_from_str(ip_wblist_t *ip_wblist, const char *str_ip);

int
ip_wblist_delete_from_int(ip_wblist_t *ip_wblist, uint32_t key, uint32_t mask);

int
ip_wblist_delete_from_str(ip_wblist_t *ip_wblist, const char *str_ip);

int
ip_wblist_find_from_int(ip_wblist_t *ip_wblist, uint32_t key);

int
ip_wblist_find_from_str(ip_wblist_t *ip_wblist, const char *str_ip);

int
ip_wblist_clear(ip_wblist_t *ip_wblist);

uint32_t
ip_wblist_get_count(ip_wblist_t *ip_wblist);

#endif
