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
#include "dig_wire_name.h"
#include "zip_record_store.h"
#include "zip_pkg_manager.h"
#include "zip_pkg_list.h"
#include "dig_thread_pool.h"
#include "pthread.h"
#include "zip_ip_wblist.h"
#include "zip_domain_wblist.h"
#include "dig_command_server.h"
#include "zip_command_runner.h"
#include "dynamic_queue.h"

#define MAX_QUERY_BUF_LEN 512


#ifndef PATH_MAX
#define PATH_MAX 1024
#endif

typedef struct dnscache
{
    struct event_base *base_event_;
    struct event *sig_int_event_;
    struct event *sig_term_event_;
    dns_server_t *dns_server_;
    record_store_t *record_store_;
    ip_wblist_t *ip_wblist_;
    domain_wblist_t *domain_wblist_;
    thread_pool_t *tp_;
    command_server_t *command_server_;
    bool cache_pattern_;
}dnscache_t;

static void 
dnscache_delete(dnscache_t *dnscache);

static void
dnscache_init_sig(dnscache_t *dnscache);

static void *
dnscache_handle_query_pkg(task_param_t *arg);

static void *
dnscache_handle_response_pkg(task_param_t *arg);

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
    dns_server_stop(dnscache->dns_server_);
    thread_pool_stop(dnscache->tp_);
    command_server_stop(dnscache->command_server_);
}

static void
handle_dns_query(dqueue_t *queue, void *arg)
{
    dnscache_t *dnscache = (dnscache_t *)arg;
    
    if (dnscache->cache_pattern_ == CACHE_QUERY)
        thread_pool_add_task_to_run(dnscache->tp_,
                                    dnscache_handle_query_pkg,
                                    queue);
    else
        thread_pool_add_task_to_run(dnscache->tp_,
                                    dnscache_handle_response_pkg,
                                    queue);
}
static void *
dnscache_handle_query_pkg(task_param_t *param)
{
    dqueue_t *queue = (dqueue_t *)param->real_time_param;
    dnscache_t *dnscache = (dnscache_t *)param->thread_local_data;
    query_session_t *session = NULL;
    while (1)
    {
        session = dqueue_dequeue(queue);
        if (NULL == session)
            break;

        if (DNS_QUERY != pkg_query_or_response(session->raw_data))
        {
            mem_pool_free(session->mp, session);
            continue;
        }
        /*---------------ip white black list----------------*/
        char ip[MAX_IP_STR_LEN]; 
        addr_get_ip(&session->client_addr, ip);
        if (0 == ip_wblist_find_from_str(dnscache->ip_wblist_, ip))
        {
            mem_pool_free(session->mp, session);
            continue;
        }
        /*---------------domain white black list----------------*/
        buffer_t buf;
        buffer_create_from(&buf, (void *)(session->raw_data + 12), session->data_len - 12); 
        wire_name_t *wire_name = wire_name_from_wire(&buf);
        if (!wire_name)
        {
            mem_pool_free(session->mp, session);
            continue;
        }
        if (domain_wblist_find_from_wire(dnscache->domain_wblist_, wire_name) == 0)
        {
            mem_pool_free(session->mp, session);
            continue;
        }

        /*---------------get cache data ----------------*/
        bool ret = record_store_get_pkg(dnscache->record_store_,
                                        session->raw_data,
                                        &session->data_len);
        if (ret)
        {
            socket_write_to(&session->client_socket,
                            (uint8_t *)session->raw_data,
                            session->data_len,
                            &session->client_addr);

            mem_pool_free(session->mp, session);
            continue;
        }

        uint8_t *header = (uint8_t *)session->raw_data;
        uint16_t flag = *((uint16_t *)header + 1);
        *((uint16_t *)header + 1) = flag | 128;

        socket_write_to(&session->client_socket,
                        (uint8_t *)session->raw_data,
                        session->data_len,
                        &session->client_addr);

        mem_pool_free(session->mp, session);
    }
}

static void *
dnscache_handle_response_pkg(task_param_t *param)
{
    dqueue_t *queue = (dqueue_t *)param->real_time_param;
    dnscache_t *dnscache = (dnscache_t *)param->thread_local_data;
    query_session_t *session = NULL;
    while (1)
    {
        session = dqueue_dequeue(queue);
        if (NULL == session)
            break;
        if (DNS_RESPONSE != pkg_query_or_response(session->raw_data))
        {
            mem_pool_free(session->mp, session);
            continue;
        }
        record_store_insert_pkg(dnscache->record_store_,
                                session->raw_data,
                                session->data_len);
        mem_pool_free(session->mp, session);
    }
}

dnscache_t * 
dnscache_create(struct event_base *base_event,
                char *server_addr,
                uint16_t port,
                uint32_t thread_num,
                uint32_t queue_size)
{

    ASSERT(base_event, "base event is NULL");
    dnscache_t *dnscache = malloc(sizeof(dnscache_t));
    if (!dnscache)
        return NULL;

    dnscache->base_event_= base_event;
    dnscache->cache_pattern_ = CACHE_WRITE;
    
    dnscache_init_sig(dnscache);

    dnscache->tp_ = thread_pool_create(thread_num);
    int i = 0;
    for (; i < thread_num; i++)
        thread_pool_set_thread_data(dnscache->tp_, i, dnscache);
    ASSERT(dnscache->tp_, "thread pool create failed");


    dnscache->record_store_ = record_store_create(1024 * 1024 * 100);
    ASSERT(dnscache->record_store_, "create record store failed");

    dnscache->ip_wblist_ = ip_wblist_create();
    ASSERT(dnscache->ip_wblist_, "create ip wblist failed in dnscache");

    dnscache->domain_wblist_ = domain_wblist_create();
    ASSERT(dnscache->domain_wblist_, "create domain wblist failed in dnscache");

    dnscache->dns_server_ = dns_server_create(dnscache->base_event_, 
                                              UDP_SERVER, 
                                              server_addr, 
                                              port,
                                              queue_size);
    ASSERT(dnscache->dns_server_, "create dns server failed\n");
    
    dnscache->command_server_ = command_server_create(dnscache->base_event_,
                                                      "127.0.0.1",
                                                      9999,
                                                      "./dnscache_command.json");
    ASSERT(dnscache->command_server_, "create command server failed\n");

    return dnscache;
}

static void
dnscache_init(dnscache_t *dnscache)
{
    command_runner_t *add_ip_runner
            = create_add_ip_wblist_runner(dnscache->ip_wblist_);
    command_server_add_command_runner(dnscache->command_server_, add_ip_runner);

    command_runner_t *add_domain_runner
            = create_add_domain_wblist_runner(dnscache->domain_wblist_);
    command_server_add_command_runner(dnscache->command_server_,
                                      add_domain_runner);

    command_runner_t *del_ip_runner
            = create_delete_ip_wblist_runner(dnscache->ip_wblist_);
    command_server_add_command_runner(dnscache->command_server_, del_ip_runner);

    command_runner_t *del_domain_runner
            = create_delete_domain_wblist_runner(dnscache->domain_wblist_);
    command_server_add_command_runner(dnscache->command_server_,
                                      del_domain_runner);
    command_runner_t *cache_pattern
            = create_cache_pattern_runner(&dnscache->cache_pattern_);
    command_server_add_command_runner(dnscache->command_server_,
                                      cache_pattern);
    command_runner_t *cache_capacity
            = create_cache_capacity_runner(dnscache->record_store_);
    command_server_add_command_runner(dnscache->command_server_,
                                      cache_capacity);
}

static void
dnscache_run(dnscache_t *dnscache)
{
    dns_server_start(dnscache->dns_server_, handle_dns_query, dnscache);
    thread_pool_start(dnscache->tp_);
    command_server_start(dnscache->command_server_);
    event_base_dispatch(dnscache->base_event_);
}

static void
dnscache_delete(dnscache_t *dnscache)
{
    event_free(dnscache->sig_int_event_);
    event_free(dnscache->sig_term_event_);
    dns_server_delete(dnscache->dns_server_);
    record_store_delete(dnscache->record_store_);
    ip_wblist_destroy(dnscache->ip_wblist_);
    domain_wblist_destroy(dnscache->domain_wblist_);
    thread_pool_delete(dnscache->tp_);
    command_server_delete(dnscache->command_server_);
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
    int record_store_size = 1024*1024;
    uint32_t queue_size = 100;

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
            case 'm':
                record_store_size = strtol(optarg, NULL, 10);
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

    struct event_base *base_event = event_base_new();
    dnscache_t *dnscache = dnscache_create(base_event,
                                           server_addr,
                                           port,
                                           thread_num,
                                           queue_size);
    dnscache_init(dnscache);
    dnscache_run(dnscache);
    dnscache_delete(dnscache);
    return 0;
}

