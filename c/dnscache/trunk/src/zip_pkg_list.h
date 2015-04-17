#ifndef _PACKAGE_LIST_HEADER_H
#define _PACKAGE_LIST_HEADER_H

#include <stdbool.h>
#include <stdint.h>
#include "dig_wire_name.h"


typedef struct pkg_data pkg_data_t;

struct pkg_list
{
    pkg_data_t      *header;
    uint32_t        count;
    uint32_t        mem_size;
    struct pkg_list *prev;
    struct pkg_list *next;
    wire_name_t *owner;
};

typedef struct pkg_list pkg_list_t;


pkg_data_t *
pkg_data_create_from(const char *raw_data, uint16_t data_len, uint16_t type);

void *
pkg_data_get_data(pkg_data_t *pkt);

uint32_t
pkg_data_mem_size(pkg_data_t *pkg);

uint16_t 
pkg_data_get_type(pkg_data_t *pkt);

uint16_t
pkg_data_get_len(pkg_data_t *pkt);

pkg_list_t *
pkg_list_create();

void
pkg_list_destroy(pkg_list_t *list);

uint32_t 
pkg_list_get_count(pkg_list_t *pkg_list);

const wire_name_t *
pkg_list_get_owner(pkg_list_t *list);

void
pkg_list_set_owner(pkg_list_t *list, wire_name_t *name);

uint32_t
pkg_list_mem_size(pkg_list_t *list);


void
pkg_list_insert(pkg_list_t *list, pkg_data_t *pkt);

pkg_data_t *
pkg_list_search(pkg_list_t *list, uint8_t query_type);

void
pkg_list_delete(pkg_list_t *list, pkg_data_t *pkg);

#endif
