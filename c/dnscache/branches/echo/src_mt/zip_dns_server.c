#include <event2/event.h>
#include <stdio.h>
#include <errno.h>
#include "zip_dns_server.h"
#include "zip_socket.h"
#include "zip_utils.h"
#include "zip_dns_lib.h"

#define TCP_MAX_PACKAGE_LEN 4096
#define INIT_MEM_SIZE 10000

typedef struct dns_udp_server
{
    socket_t *server_socket_;
    dns_package_handler_t handler_;
    void *user_data_;
    link_queue_t *queue_;
    mem_pool_t *mp_;

    struct event_base *base_event_;
    struct event *udp_read_event_;
}dns_udp_server_t;

struct dns_server
{
    server_t server_type_;
    dns_udp_server_t *udp_server_;
};

static dns_udp_server_t*
dns_udp_server_create(struct event_base *base,
                      socket_t *socket,
                      uint16_t queue_size);

static void
dns_udp_server_delete(dns_udp_server_t *udp_server);

static void
dns_udp_server_start(dns_udp_server_t *server,
                     dns_package_handler_t handler,
                     void *user_data);

static void
dns_udp_server_stop(dns_udp_server_t *server);

static void
dns_udp_server_recv_data_call_back(evutil_socket_t fd, short event, void *arg);

dns_server_t *
dns_server_create(struct event_base *base,
                  server_t type,
                  socket_t *socket,
                  uint16_t queue_size)
{
    dns_server_t *server = (dns_server_t *)calloc(1, sizeof(dns_server_t));
    if (server)
    {
        server->server_type_ = type;
        if (type == UDP_SERVER)
        {
            server->udp_server_ = dns_udp_server_create(base, socket, queue_size);
        }
    }

    return server;
}

static dns_udp_server_t *
dns_udp_server_create(struct event_base *base,
                      socket_t *socket,
                      uint16_t queue_size)
{
    dns_udp_server_t *server = malloc(sizeof(dns_udp_server_t));
    if (server)
    {
        server->server_socket_ = socket;
        server->queue_ = dqueue_create_with_size(queue_size);
        server->mp_ = mem_pool_create(sizeof(query_session_t), INIT_MEM_SIZE, true);
        server->base_event_ = base;
    }

    return server;
}

void
dns_server_delete(dns_server_t *server)
{
    if (server->server_type_ == UDP_SERVER)
    {
        dns_udp_server_delete(server->udp_server_);
    }
    free(server);
}

static void
dns_udp_server_delete(dns_udp_server_t *server)
{
    event_free(server->udp_read_event_);
    mem_pool_delete(server->mp_);
    dqueue_destroy(server->queue_);
    free(server);
}

void
dns_server_start(dns_server_t *server,
                 dns_package_handler_t handler,
                 void *user_data)
{
    if (server->server_type_ == UDP_SERVER)
        dns_udp_server_start(server->udp_server_, handler, user_data);
}

static void
dns_udp_server_start(dns_udp_server_t *server,
                     dns_package_handler_t handler,
                     void *user_data)
{
    server->handler_ = handler;
    server->user_data_ = user_data;

    server->udp_read_event_ = event_new(server->base_event_,
                                        SOCKET_FD(server->server_socket_),
                                        EV_READ|EV_PERSIST,
                                        dns_udp_server_recv_data_call_back,
                                        (void*)server);
    if (server->udp_read_event_ == NULL)
    {
        perror("udp read event create failed\n");
    }

    event_add(server->udp_read_event_, NULL);
    event_base_dispatch(server->base_event_);
    return;
}

bool
query_pkg_is_valid(const char *raw_data, uint16_t data_len)
{
    if (data_len < 12 + 4 + 1 || data_len > 512)
        return false;

    if (dns_header_get_opt(raw_data) != QUERY_STAND)
        return false;

    return true;
}

static void
dns_udp_server_recv_data_call_back(evutil_socket_t fd, short event, void *arg)
{
    dns_udp_server_t *server = (dns_udp_server_t*)arg;

    while(!dqueue_full(server->queue_))
    {
        query_session_t *session = mem_pool_alloc(server->mp_);
        if (!session)
            break;

        session->client_socket = server->server_socket_;
        int len = socket_read_from(session->client_socket,
                                   session->raw_data,
                                   UDP_MAX_PACKAGE_LEN,
                                   &session->client_addr);
        session->data_len = len;
        if (len <= 0)
        {
            mem_pool_free(server->mp_, session);
            break;
        }
        else
        {
            if (query_pkg_is_valid(session->raw_data, session->data_len))
            {
                session->mp = server->mp_; 
                dqueue_enqueue(server->queue_, session); 
            }
            else
            {
                mem_pool_free(server->mp_, session);
            }
        }
    }

    (server->handler_)(server->queue_, server->user_data_);
}

void
dns_server_stop(dns_server_t *server)
{
    if (server->server_type_ == UDP_SERVER)
    {
        dns_udp_server_stop(server->udp_server_);
    }
}

static void
dns_udp_server_stop(dns_udp_server_t *server)
{
    if (server->udp_read_event_)
        event_del(server->udp_read_event_);
}
