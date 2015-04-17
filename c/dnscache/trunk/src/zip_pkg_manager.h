#ifndef _PACKAGE_MANAGER_HEADER_H
#define _PACKAGE_MANAGER_HEADER_H

#include <stdint.h>
#include "zip_pkg_list.h"

enum
{
    DNS_QUERY = 0,
    DNS_RESPONSE
};

uint16_t
pkg_query_or_response(const char *raw_data);

uint16_t
pkg_get_query_type(const char *raw_data, uint16_t data_len);

uint16_t
pkg_get_query_id(const char *raw_data);

void
pkg_set_response_id(char *pkg_data, uint16_t id);


#endif
