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
    addr_t server_address_;
    socket_t server_socket_;
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
dns_udp_server_create(struct event_base *base, const char *ip, uint16_t port, uint16_t queue_size);

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
                  const char *ip,
                  uint16_t port,
                  uint16_t queue_size)
{
    dns_server_t *server = (dns_server_t *)calloc(1, sizeof(dns_server_t));
    if (server)
    {
        server->server_type_ = type;
        if (type == UDP_SERVER)
        {
            server->udp_server_ = dns_udp_server_create(base, ip, port, queue_size);
        }
    }

    return server;
}

static dns_udp_server_t *
dns_udp_server_create(struct event_base *base, const char *ip, uint16_t port, uint16_t queue_size)
{
    dns_udp_server_t *server = malloc(sizeof(dns_udp_server_t));
    if (server)
    {
        addr_init(&server->server_address_, ip, port);
        server->queue_ = dqueue_create_with_size(queue_size);
        server->mp_ = mem_pool_create(sizeof(query_session_t), INIT_MEM_SIZE, 10000);
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
    socket_close(&server->server_socket_);
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

    const addr_t *addr = &server->server_address_;
    socket_t *listen_socket = &server->server_socket_;
    int socket_type = addr_get_type(addr) == ADDR_IPV4 ? AF_INET : AF_INET6;
    socket_open(listen_socket, socket_type, SOCK_DGRAM, 0);
    socket_set_unblock(listen_socket, true);
    socket_set_addr_reusable(listen_socket);

    int ret = socket_bind(listen_socket, addr);
    if (ret != 0)
    {
        perror("udp server bind failed\n");
        goto START_FAILED;
    }
    int i = 0;
    for (;i < 1; i++)
    {
        pid_t pid = fork();
        switch(pid)
        {
        case -1:
            printf("fork error\n");
            break;
        case 0:
            printf("child process id is %d\n", getpid());
            break;
        default: 
            printf("father process id is %d, child is %d\n", getpid(), pid);
            break;
        }
    }

    server->udp_read_event_ = event_new(server->base_event_,
                                        SOCKET_FD(listen_socket),
                                        EV_READ|EV_PERSIST,
                                        dns_udp_server_recv_data_call_back,
                                        (void*)server);
    if (server->udp_read_event_ == NULL)
    {
        perror("udp read event create failed\n");
        goto START_FAILED;
    }

    event_add(server->udp_read_event_, NULL);
    return;

START_FAILED:
    socket_close(listen_socket);
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
    static int  count = 0;

    while(!dqueue_full(server->queue_))
    {
        query_session_t *session = mem_pool_alloc(server->mp_);
        if (!session)
        {
            printf("no alloc\n");
            break;
        }

        session->client_socket = server->server_socket_;
        int len = socket_read_from(&session->client_socket,
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
