#include <stdlib.h>
#include <string.h>
#include "zip_dns_lib.h"

int
wire_name_to_human_name(const char *wire_name,
                        char *human_name,
                        uint16_t wire_name_len)
{
    char label_len = 0;
    uint16_t human_name_pos = 0;
    uint16_t wire_name_pos = 0;
    while ((label_len = wire_name[wire_name_pos]) != 0)
    {
        if (label_len > MAX_LABEL_LEN)
            return -1;
        if (wire_name_pos > MAX_DOMAIN_NAME_LEN)
            return -1;
        if (wire_name_pos + label_len > wire_name_len)
            return -1;

        memcpy(human_name + human_name_pos,
               wire_name + wire_name_pos + 1,
               label_len);
        human_name_pos += label_len;
        human_name[human_name_pos++] = '.';
        wire_name_pos += label_len + 1;
    }

    human_name[human_name_pos] = 0;

    return 0;
}

#define OPCODE_MASK 0x7800
opcode_t
dns_header_get_opt(const uint8_t *header)
{
    uint16_t flag = ntohs(*((uint16_t *)header + 1));
    return flag & OPCODE_MASK;
}

void
dns_header_set_opt(uint8_t *header, opcode_t opcode)
{
    uint16_t flag = ntohs(*((uint16_t *)header + 1));
    flag |= opcode;
    *((uint16_t *)header + 1) = htons(flag);
}

#if 0
#include <stdio.h>
int main()
{
    char wire_name[] = {5,'b','a','i','d','u',3,'c','o','m',0};
    char human_name[20];
    if (wire_name_to_human_name(wire_name, human_name, sizeof(wire_name)) == 0)
        printf("human name is %s\n", human_name);
    else
        printf("trnasfer failed\n");

    return 0;
}
#endif
