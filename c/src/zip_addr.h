#ifndef DIG_ADDR_H
#define DIG_ADDR_H

#include <sys/socket.h>
#include <stdbool.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdint.h>

#define MAX_IP_STR_LEN 64

typedef enum {ADDR_IPV4, ADDR_IPV6} addr_type_t;
typedef struct _addr
{
    union
    {
        struct sockaddr_in addr_v4;
        struct sockaddr_in6 addr_v6;
    };
    int mask;
    addr_type_t addr_type;

}addr_t;

void
addr_init(addr_t *addr, const char *ip, uint16_t port);

void
addr_from_sockaddr(addr_t *addr, struct sockaddr *sock_addr, int socklen);

inline addr_type_t
addr_get_type(const addr_t *addr);

inline socklen_t
addr_get_socklen(const addr_t *addr);

inline struct sockaddr *
addr_to_sockaddr(const addr_t *addr);

void
addr_get_ip(const addr_t *addr, char *ip);

uint16_t
addr_get_port(const addr_t *addr);

void
addr_set_port(addr_t *addr, uint16_t port);

void
addr_set_mask(addr_t *addr, int mask);

int
addr_get_mask(const addr_t *addr);

bool
addr_is_in_same_network(const addr_t *addr1, const addr_t *addr2);

bool
addr_is_loopback(const addr_t *addr);

uint32_t
addr_get_v4_addr(const addr_t *addr);

#endif

