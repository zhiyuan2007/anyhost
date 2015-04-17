#ifndef DIG_SERVER_H
#define DIG_SERVER_H
#include <stdint.h>
#include "zip_socket.h"
#include "zip_addr.h"
#include "dig_mem_pool.h"
#include "dynamic_queue.h"

#define UDP_MAX_PACKAGE_LEN 512

typedef struct dns_server dns_server_t;
typedef enum
{
    UDP_SERVER,
    TCP_SERVER
}server_t;

typedef struct query_session
{
    uint8_t raw_data[UDP_MAX_PACKAGE_LEN];
    uint16_t data_len;
    socket_t client_socket;
    addr_t client_addr;
    mem_pool_t *mp;
}query_session_t;

/*
 * notes: for tcp server, currently we can only read request from end user
 * we needs to pass session id to call back and provide interface to let user write data
 * back through the session, but this will involve user state management we will add it later.
 * */
typedef void(*dns_package_handler_t)(dqueue_t *queue, void *user_data);

dns_server_t *
dns_server_create(struct event_base *base,
                  server_t type,
                  const char *ip,
                  uint16_t port,
                  uint16_t queue_size);

void
dns_server_delete(dns_server_t *server);

void
dns_server_start(dns_server_t *server,
                 dns_package_handler_t hander,
                 void *user_data);

void
dns_server_stop(dns_server_t *server);

#endif

