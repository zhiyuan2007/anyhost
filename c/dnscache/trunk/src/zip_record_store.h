#ifndef _DNS_DATA_STORE_HEADER_H
#define _DNS_DATA_STORE_HEADER_H

#include "dig_buffer.h"
#include "dig_wire_name.h"
#include "zip_pkg_list.h"
typedef struct record_store record_store_t;

record_store_t *
record_store_create(uint32_t mem_top_limit);

void
record_store_delete(record_store_t *record_store);

void
record_store_set_capacity(record_store_t *record_store, uint32_t capacity);

void
record_store_insert_pkg(record_store_t *record_store,
                        const char *raw_data,
                        const uint16_t data_len);

bool
record_store_get_pkg(record_store_t *record_store,
                    char *raw_data,
                    uint16_t *data_len);
#endif
