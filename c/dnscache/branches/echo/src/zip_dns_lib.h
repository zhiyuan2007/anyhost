#ifndef ZIP_DNS_LIB_
#define ZIP_DNS_LIB_

#include <stdint.h>

#define MAX_DOMAIN_NAME_LEN 256
#define MAX_LABEL_LEN 63

typedef enum
{
    QUERY_STAND   = 0x0000,   //< 0: Standard query (RFC1035)
    QUERY_INVERSE = 0x0800,   //< 1: Inverse query (RFC1035)
    QUERY_STATUS  = 0x1000,   //< 2: Server status request (RFC1035)
    QUERY_NOTIFY  = 0x2000,   //< 4: Notify (RFC1996)
    QUERY_UPDATE  = 0x2800    //< 5: Dynamic update (RFC2136)
} opcode_t;

int
wire_name_to_human_name(const char *wire_name,
                        char *human_name,
                        uint16_t wire_name_len);

opcode_t
dns_header_get_opt(const uint8_t *header);

void
dns_header_set_opt(uint8_t *haeder, opcode_t opcode);

#endif
