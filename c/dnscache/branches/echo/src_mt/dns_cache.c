#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/time.h>

#include <event2/event.h>
#include <event2/event_struct.h>

#if (HAVE_CONFIG_H)
#include "config.h"
#endif

#include "zip_utils.h"
#include "zip_dns_lib.h"
#include "zip_dns_server.h"
#include "dig_mem_pool.h"
#include "dig_thread_pool.h"
#include "pthread.h"
#include "dynamic_queue.h"

#define MAX_QUERY_BUF_LEN 512
#define TIMEOUT 4294967296


#ifndef PATH_MAX
#define PATH_MAX 1024
#endif

void *multi_thread_task(task_param_t *param);

typedef struct thread_local_parameter
{
    int thread_id_;
    struct event_base *base_;
    dns_server_t *dns_server_;

}thread_local_param_t;

typedef struct dnscache
{
    struct event_base *base_event_;
    struct event *sig_int_event_;
    struct event *sig_term_event_;
    thread_local_param_t **thread_params_;
    thread_pool_t *tp_;
    socket_t *listen_socket;
    int thread_count_;
}dnscache_t;

static void 
dnscache_delete(dnscache_t *dnscache);

static void
dnscache_init_sig(dnscache_t *dnscache);

static void
timeout_cb(evutil_socket_t fd, short event, void *arg)
{
    return;
}

static void
dns_cache_print_help()
{
    fprintf(stderr, "Usage: dns_cache [OPTION]...\n");
    fprintf(stderr,
            "Supported options:\n"
            "  -s             bind addr.\n"
            "  -p             listen on port.\n"
            "  -n             startup thread numbers. default is 1\n"
            "  -m             initial record store memory size. default 1024k"
            "  -v             Print version information.\n");

    fprintf(stderr, "Version %s. Report bugs to <%s>.\n",
            PACKAGE_VERSION, PACKAGE_BUGREPORT);

}

static void
dns_cache_print_version(void)
{
    fprintf(stderr, "%s version %s\n", PACKAGE_NAME, PACKAGE_VERSION);
    fprintf(stderr, "Written by knet.\n");
    fprintf(stderr,
            "Copyright (C) 2011 knet.\n");
    exit(0);
}


static void
stop_call_back(evutil_socket_t fd, short event, void *arg)
{
    dnscache_t *dnscache = (dnscache_t*)arg;
    event_del(dnscache->sig_int_event_);
    event_del(dnscache->sig_term_event_);
    thread_pool_stop(dnscache->tp_);
}

static int queue_size = 10;

void
handle_dns_query(link_queue_t *queue, void *arg)
{
    query_session_t *session = NULL;
    while((session = dqueue_dequeue(queue)) != NULL)
    {
        uint8_t *header = (uint8_t *)session->raw_data;
        uint16_t flag = *((uint16_t *)header + 1);
        *((uint16_t *)header + 1) = flag | 128;

        int len = socket_write_to(session->client_socket,
                        (uint8_t *)session->raw_data,
                        session->data_len,
                        &session->client_addr);
        mem_pool_free(session->mp, session);
    }
}

void
*multi_thread_task(task_param_t *param)
{
    task_param_t *tp = (task_param_t *)param;
    thread_local_param_t *tlp = (thread_local_param_t *)tp->thread_local_data;

    dns_server_start(tlp->dns_server_, handle_dns_query, NULL);
}

dnscache_t * 
dnscache_create(char *server_addr,
                uint16_t port,
                uint32_t thread_num)
{
    dnscache_t *dnscache = malloc(sizeof(dnscache_t));
    if (!dnscache)
        return NULL;

    dnscache->thread_count_ = thread_num;
    dnscache->base_event_ = event_base_new();
    dnscache_init_sig(dnscache);

    dnscache->tp_ = thread_pool_create(thread_num);
    ASSERT(dnscache->tp_, "thread pool create failed");

    addr_t addr;
    addr_init(&addr, server_addr, port);
    dnscache->listen_socket = (socket_t *)malloc(sizeof(socket_t));
    int socket_type = addr_get_type(&addr) == ADDR_IPV4 ? AF_INET : AF_INET6;
    socket_open(dnscache->listen_socket, socket_type, SOCK_DGRAM, 0);
    socket_set_unblock(dnscache->listen_socket, true);
    socket_set_addr_reusable(dnscache->listen_socket);
    int ret = socket_bind(dnscache->listen_socket, &addr);
    ASSERT(ret == 0, "udp server bind failed\n"); 

    int threads_len = thread_num * sizeof(thread_local_param_t *);
    dnscache->thread_params_ = (thread_local_param_t **)malloc(threads_len);
    ASSERT(dnscache->thread_params_  != NULL, "udp server bind failed\n"); 
    int i = 0;

    for (; i < thread_num; ++i)
    {
        dnscache->thread_params_[i] =
            (thread_local_param_t *)malloc(sizeof(thread_local_param_t));
        thread_local_param_t *tlp = dnscache->thread_params_[i];
        tlp->thread_id_ = i;
        tlp->base_ = event_base_new();
        tlp->dns_server_ =
            dns_server_create(tlp->base_, UDP_SERVER, dnscache->listen_socket, queue_size);
        thread_pool_set_thread_data(dnscache->tp_, i, tlp);
    }
    
    return dnscache;
}

void
thread_start(dnscache_t *dnscache)
{
    int i = 0;
    
    for (; i < dnscache->thread_count_; ++i)
    {
        thread_pool_add_task_to_run(dnscache->tp_, multi_thread_task, NULL);
    }
}

void
thread_delete(dnscache_t *dnscache)
{
    int i = 0;
    for (; i < dnscache-> thread_count_; ++i)
    {
        thread_local_param_t *tlp = dnscache->thread_params_[i];
        event_base_free(tlp->base_);
        dns_server_delete(tlp->dns_server_);
    }
}

static void
dnscache_run(dnscache_t *dnscache)
{
    thread_pool_start(dnscache->tp_);
    thread_start(dnscache);
    struct timeval tv;
    struct event timeout;
    event_assign(&timeout, dnscache->base_event_, -1, EV_PERSIST, timeout_cb, NULL);
    evutil_timerclear(&tv);
    tv.tv_sec = TIMEOUT;
    event_add(&timeout, &tv);
    event_base_dispatch(dnscache->base_event_);
}

static void
dnscache_delete(dnscache_t *dnscache)
{
    event_free(dnscache->sig_int_event_);
    event_free(dnscache->sig_term_event_);
    thread_delete(dnscache);
    thread_pool_delete(dnscache->tp_);
    free(dnscache->listen_socket);
    free(dnscache);
}

static void
dnscache_init_sig(dnscache_t *dnscache)
{
    struct sigaction sighandler;
    sighandler.sa_handler = SIG_IGN;
    sighandler.sa_flags = 0;
    sigfillset(&sighandler.sa_mask);

    int ret = sigaction(SIGPIPE, &sighandler, NULL);
    ASSERT (ret == 0, "sigaction failed");
    ret = sigaction(SIGHUP, &sighandler, NULL);
    ASSERT (ret == 0, "sigaction failed");

    dnscache->sig_int_event_ = evsignal_new(dnscache->base_event_, 
                                            SIGINT, 
                                            stop_call_back, 
                                            dnscache);

    dnscache->sig_term_event_ = evsignal_new(dnscache->base_event_, 
                                             SIGTERM, 
                                             stop_call_back, 
                                             dnscache);
    event_add(dnscache->sig_int_event_, NULL);
    event_add(dnscache->sig_term_event_, NULL);
}

int main(int argc, char *argv[])
{
    int thread_num = 1;

    char server_addr[MAX_IP_STR_LEN] = "0.0.0.0";
    uint16_t port = 5053;

    int opt = 0;
    while ((opt = getopt(argc, argv, "s:p:n:m:q:")) != -1)
    {
        switch(opt)
        {
            case 's':
                strncpy(server_addr, optarg, MAX_IP_STR_LEN);
                break;
            case 'p':
                port = strtol(optarg, NULL, 10);
                break;
            case 'h':
                dns_cache_print_help();
                exit(0);
            case 'n':
                thread_num = strtol(optarg, NULL, 10);
                break;
            case 'q':
                queue_size = strtol(optarg, NULL, 10);
                break;
            case 'v':
                dns_cache_print_version();
                exit(0);
            default:
                dns_cache_print_help();
                exit(0);
        }
    }

    dnscache_t *dnscache = dnscache_create(server_addr,
                                           port,
                                           thread_num
                                           );
    dnscache_run(dnscache);
    dnscache_delete(dnscache);
    return 0;
}

