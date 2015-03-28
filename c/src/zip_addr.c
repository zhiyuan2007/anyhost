#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include "zip_addr.h"
#include "log_utils.h"

void
addr_init(addr_t *addr, const char *ip, uint16_t port)
{
    addr->addr_type = strchr(ip, ':') == NULL ? ADDR_IPV4 : ADDR_IPV6;
    int ret;

    if (addr->addr_type == ADDR_IPV4)
    {
        bzero(&addr->addr_v4, sizeof(struct sockaddr_in));
        addr->addr_v4.sin_family = AF_INET;
        addr->addr_v4.sin_port = htons(port);
        ret = inet_pton(AF_INET, ip, &(addr->addr_v4.sin_addr));
        ASSERT(ret == 1, "ip address %s isn't valid ip", ip);
        addr->mask = 32;
    }
    else
    {
        bzero(&addr->addr_v6, sizeof(struct sockaddr_in6));
        addr->addr_v6.sin6_family = AF_INET6;
        addr->addr_v6.sin6_port = htons(port);
        ret = inet_pton(AF_INET6, ip, &(addr->addr_v6.sin6_addr));
        ASSERT(ret == 1, "ip address %s isn't valid ip", ip);
        addr->mask = 128;
    }

}


inline addr_type_t
addr_get_type(const addr_t *addr)
{
    return addr->addr_type;
}

inline socklen_t
addr_get_socklen(const addr_t *addr)
{
    return addr_get_type(addr) ==
        ADDR_IPV4 ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6);
}

inline struct sockaddr *
addr_to_sockaddr(const addr_t *addr)
{
    return addr_get_type(addr) ==
    ADDR_IPV4 ? (struct sockaddr *)&addr->addr_v4
              : (struct sockaddr *)&addr->addr_v6;
}

void
addr_from_sockaddr(addr_t *addr, struct sockaddr *sock_addr, int socklen)
{
    if (socklen == sizeof(struct sockaddr_in))
    {
        addr->addr_type = ADDR_IPV4;
        addr->addr_v4 = *(struct sockaddr_in *)sock_addr;
        addr->mask = 32;
    }
    else if (socklen == sizeof(struct sockaddr_in6))
    {
        addr->addr_type = ADDR_IPV6;
        addr->addr_v6 = *(struct sockaddr_in6 *)sock_addr;
        addr->mask = 128;
    }
    else
        ASSERT(0, "unknow sock type\n");
}

uint16_t
addr_get_port(const addr_t *addr)
{

    return addr_get_type(addr) == ADDR_IPV4 ? ntohs(addr->addr_v4.sin_port) :
        ntohs(addr->addr_v6.sin6_port);
}

void
addr_set_port(addr_t *addr, uint16_t port)
{
    if (addr_get_type(addr) == ADDR_IPV4)
        addr->addr_v4.sin_port = htons(port);
    else
        addr->addr_v6.sin6_port = htons(port);

}

void
addr_get_ip(const addr_t *addr, char *ip)
{
    if (addr_get_type(addr) == ADDR_IPV4)
        inet_ntop(AF_INET, &(addr->addr_v4.sin_addr), ip, MAX_IP_STR_LEN);
    else
        inet_ntop(AF_INET6, &(addr->addr_v6.sin6_addr), ip, MAX_IP_STR_LEN);
}


inline void
addr_set_mask(addr_t *addr, int mask)
{
    addr->mask = mask;
}


inline int
addr_get_mask(const addr_t *addr)
{
    return addr->mask;
}


bool
addr_is_in_same_network(const addr_t *addr1, const addr_t *addr2)
{
    int mask = addr1->mask < addr2->mask ? addr1->mask : addr2->mask;
    addr_type_t addr1_type = addr_get_type(addr1);
    addr_type_t addr2_type = addr_get_type(addr2);
    if (addr1_type != addr2_type)
        return false;

    if (mask < 0)
        mask = 0;

    if (ADDR_IPV4 == addr1_type)
    {
        if (mask > 32) mask = 32;
        uint32_t addr1_ip = ntohl(addr1->addr_v4.sin_addr.s_addr);
        uint32_t addr2_ip = ntohl(addr2->addr_v4.sin_addr.s_addr);
        uint32_t m = (uint32_t)(-(1 << (32 - mask)));
        return (addr1_ip & m) == (addr2_ip & m);

    }
    else
    {
        if (mask > 128) mask = 128;
        int i = 0;
        while (mask >= 0)
        {
            uint8_t addr1_section = ntohs(addr1->addr_v6.sin6_addr.s6_addr[i]);
            uint8_t addr2_section = ntohs(addr2->addr_v6.sin6_addr.s6_addr[i]);
            if (mask < 8)
            {
                uint16_t m = (uint16_t)(-(1<<(8-mask)));
                return (addr1_section & m) == (addr2_section & m);
            }
            else
            {
                if (addr1_section != addr2_section)
                    return false;
                mask -= 8;
            }
            ++i;

        }
    }

    return true;
}

bool
addr_is_loopback(const addr_t *addr)
{
    static const char *IPV4_LOOPBACK = "127.0.0.1";
    static const char *IPV6_LOOPBACK = "0:0:0:0:0:0:0:1";
    static const char *IPV6_LOOPBACK_SIMPLE = "::1";
    char ip[MAX_IP_STR_LEN];
    addr_get_ip(addr, ip);
    if (addr_get_type(addr) == ADDR_IPV4)
        return strcmp(ip, IPV4_LOOPBACK) == 0;
    else
        return strcmp(ip, IPV6_LOOPBACK) == 0 || strcmp(ip, IPV6_LOOPBACK_SIMPLE) == 0;
}

int
addr_compare(const addr_t *src, const addr_t *dst)
{
    char ip1[MAX_IP_STR_LEN];
    char ip2[MAX_IP_STR_LEN];
    int ret;

    addr_get_ip(src, ip1);
    addr_get_ip(dst, ip2);

    if (strcmp(ip1, ip2) == 0)
        ret = true;
    else
        ret = false;

    return ret;
}

uint32_t
addr_get_v4_addr(const addr_t *addr)
{
    ASSERT(addr->addr_type == ADDR_IPV4, "get v4 addr from v6 addr");
    return addr->addr_v4.sin_addr.s_addr;
}
