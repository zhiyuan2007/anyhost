#include "zip_pkg_manager.h"

#include <string.h>
#define QR_MASK 0x8000

uint16_t
pkg_query_or_response(const char *raw_data)
{
    uint16_t flag = ntohs(*((uint16_t *)(raw_data+2)));
    return !!(flag & QR_MASK);
}

uint16_t
pkg_get_query_type(const char *raw_data, uint16_t data_len)
{
    uint16_t type;
    if (pkg_query_or_response(raw_data) == DNS_QUERY)
    {
        type = ntohs(*((uint16_t *)(raw_data + data_len - 4)));
    }
    else
    {
        uint8_t *pos = (uint8_t *)(raw_data + 12);
        while (*pos != 0)
            pos = pos + *pos + 1;

        type = ntohs(*((uint16_t *)(pos + 1)));
    }
    return type;
}

uint16_t
pkg_get_query_id(const char *raw_data)
{
    return ntohs(*(uint16_t*)raw_data);
}

void
pkg_set_response_id(char *pkg_data, uint16_t id)
{
    *((uint16_t*)(pkg_data)) = htons(id);
}

