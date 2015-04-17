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
#include "zip_pid.h"
#include "zip_location.h"
#include "zip_logger.h"
#include "zip_dns_lib.h"
#include "zip_dns_client.h"
#include "zip_dns_server.h"
#include "zip_dns_client.h"
#include "zip_rule_store.h"
#include "zip_ip_store.h"
#include "zip_conf.h"
#include "zip_command_server.h"
#include "zip_runner.h"
#include "zip_memory_pool.h"
#include "zip_wire_name.h"
#include "zip_buffer.h"

#define MAX_QUERY_BUF_LEN 512

#ifndef PATH_MAX
#define PATH_MAX 1024
#endif

static char dnscache_ip[16];
static int dnscache_port;

typedef struct zipper
{
    struct event_base *base_event_;

    dns_server_t *dns_server_;
    dns_client_t *dns_client_;
    rule_store_t *rule_store_;
    ip_store_t *ip_store_;
    config_t *config_;
    command_server_t *command_server_;
    char pid_file_[PATH_MAX];

    struct event *sig_int_event_;
    struct event *sig_term_event_;

    memory_pool_t *query_session_pool_;
}zipper_t;


typedef struct query_session
{
    char query_raw_data_[MAX_QUERY_BUF_LEN];
    uint16_t query_data_len_;

    wire_name_t *query_wire_name_;
    socket_t client_socket_;
    addr_t client_addr_;
    location_t *location_info_;
    zipper_t *zipper_;
}query_session_t;

static void
handle_dns_query(const uint8_t *raw_data,
                 uint16_t data_len,
                 socket_t *client_socket,
                 const addr_t *client_addr,
                 void *arg);

static void
query_named_with_view(char *view, void *arg);

static void
reply_to_end_user(dns_client_error_code_t error_code,
                  const char *dns_raw_data,
                  uint16_t raw_data_len,
                  const addr_t *name_server_addr,
                  void *user_data);

static zipper_t *
zipper_create();

static void
zipper_delete(zipper_t *zipper);

static void
zipper_init_sig(zipper_t *zipper);

static void
zipper_init_command_server(zipper_t *zipper, const char *config_path);

static void
zipper_init_log(zipper_t *zipper);

static void
zipper_init_server(zipper_t *zipper);

static void
zipper_init_config(zipper_t *zipper, const char *config_file_path);

static void
zipper_run(zipper_t *zipper, bool run_foreground);

static void
zipper_print_help();

static void
zipper_print_version();

static void
zipper_stop_call_back(evutil_socket_t fd, short event, void *arg);

static zipper_t *
zipper_create(const char *config_file_path)
{
    zipper_t *zipper = malloc(sizeof(zipper_t));
    if (zipper)
    {
        zipper->base_event_ = event_base_new();
        zrb_init_ruby_env(); 
        zipper_init_config(zipper, config_file_path);
        zipper_init_log(zipper);
        zipper->ip_store_ = ipstore_create();
        zipper->rule_store_ = rule_store_init();
        zipper_init_command_server(zipper, config_file_path);
        zipper_init_sig(zipper);
        zipper_init_server(zipper);
        zipper->query_session_pool_ = memorypool_create(sizeof(query_session_t), 100, 0);
    }

    return zipper;
}

static void
zipper_delete(zipper_t *zipper)
{
    log_fini();
    conf_destroy(zipper->config_);
    dns_server_delete(zipper->dns_server_);
    dns_client_delete(zipper->dns_client_);
    rule_store_destroy(zipper->rule_store_);
    ipstore_destroy(zipper->ip_store_);
    command_server_delete(zipper->command_server_);
    memorypool_delete(zipper->query_session_pool_);
    free(zipper);
    zrb_finalize_ruby_env();
}

static void
zipper_run(zipper_t *zipper, bool run_foreground)
{
    char ip_store_file_path[PATH_MAX]; 
    if (0 != conf_get_path(zipper->config_,
                           "ip_store_file_path", ip_store_file_path))
    {
        ASSERT(0, "log file path isn't configured\n", ip_store_file_path);
        return;
    }

    if (0 != ipstore_load(zipper->ip_store_, ip_store_file_path))
    {
        ASSERT(0, "load ip store file [%s] faild\n", ip_store_file_path);
    }

    if (!run_foreground)
    {
        int ret = fork();
        ASSERT (ret >= 0, "fork failed");
        if (ret > 0) {
            pid_file_write(zipper->pid_file_, ret);
            exit(0);
        }
        ret = setsid();
        ASSERT( ret != -1, "setsid failed");
        int fd = open("/dev/null", O_RDWR, 0);
        ASSERT(fd != -1, "open null dev failed");
        (void)dup2(fd, STDIN_FILENO);
        (void)dup2(fd, STDOUT_FILENO);
        (void)dup2(fd, STDERR_FILENO);
        if (fd > 2)
            (void)close(fd);
    }

    dns_server_start(zipper->dns_server_, handle_dns_query, zipper);
    command_server_start(zipper->command_server_);
    event_base_dispatch(zipper->base_event_);
}

static void
zipper_stop_call_back(evutil_socket_t fd, short event, void *arg)
{
    zipper_t *zipper = (zipper_t *)arg;
    event_del(zipper->sig_int_event_);
    event_del(zipper->sig_term_event_);
    event_free(zipper->sig_int_event_);
    event_free(zipper->sig_term_event_);
    dns_server_stop(zipper->dns_server_);
    command_server_stop(zipper->command_server_);
}

static void
handle_dns_query(const uint8_t *raw_data,
                 uint16_t data_len,
                 socket_t *client_socket,
                 const addr_t *client_addr,
                 void *arg)
{
    //header is 12 octets, wire name is 1 octets,
    //class is 2 octets, type is 2 octets
    if (data_len < 12 + 4 + 1)
    {
        log_warning(QUERY_LOG, "get invalid query package\n");
        return;
    }

    if (dns_header_get_opt(raw_data) != QUERY_STAND)
    {
        log_warning(QUERY_LOG, "get non-query package\n");
        return;
    }

    zipper_t *zipper = (zipper_t *)arg;

    query_session_t *session = memorypool_alloc_node(zipper->query_session_pool_);
    if (session == NULL)
    {
        log_warning(QUERY_LOG, "no memory left to handle more query\n");
        return;
    }

    memcpy(session->query_raw_data_, raw_data, data_len);
    session->query_data_len_ = data_len;
    session->zipper_ = zipper;
    session->client_socket_ = *client_socket;
    session->client_addr_ = *client_addr;

    char ip_addr_str[MAX_IP_STR_LEN];
    addr_get_ip(client_addr, ip_addr_str);
    log_debug(QUERY_LOG, "get query from ip [%s]\n", ip_addr_str);

    session->location_info_ = ipstore_get_location_use_int(zipper->ip_store_,
                                                 addr_get_v4_addr(client_addr));
    if (!session->location_info_)
    {
        log_warning(QUERY_LOG, "get ip failed\n");
        goto BAD_QUERY;
    }

    buffer_t buf;
    buffer_create_from(&buf, (void *)(raw_data + 12), data_len - 12 - 4);
    session->query_wire_name_ = wire_name_from_wire(&buf);
    if (session->query_wire_name_)
    {
        buffer_t name_buf;
        char str_name[MAX_DOMAIN_NAME_LEN] = {0};
        buffer_create_from(&name_buf, str_name, MAX_DOMAIN_NAME_LEN);
        wire_name_to_text(session->query_wire_name_, &name_buf); 
        log_debug(QUERY_LOG, "handle query name [%s]\n", str_name);
                                                
        char view_name[MAX_VIEW_NAME_LEN];
        rule_store_get_view(zipper->rule_store_, session->query_wire_name_,
                                                 session->location_info_, view_name);
        query_named_with_view(view_name, session); 
        wire_name_delete(session->query_wire_name_);
    }
    else
    {
        wire_name_delete(session->query_wire_name_);
        log_warning(QUERY_LOG, "get invalid query name\n");
        goto BAD_QUERY;
    }
    return;

BAD_QUERY:
    memorypool_free_node(zipper->query_session_pool_, session);
}

void
query_named_with_view(char *view, void *arg)
{
    query_session_t *session = (query_session_t *)arg;

    int view_len = strlen(view);
    if (view_len > MAX_VIEW_NAME_LEN
        || session->query_data_len_ + view_len + 1 > MAX_QUERY_BUF_LEN)
    {
        log_warning(QUERY_LOG,
                 "view is too long or query data plus view bigger than 512\n");
        memorypool_free_node(session->zipper_->query_session_pool_, session);
        return;
    }

    //notify named, this query isn't normal query but carry view info
    dns_header_set_opt(session->query_raw_data_, QUERY_INVERSE);
    //append the view name to query data, the ending zero of view name is ommited
    memcpy(session->query_raw_data_ + session->query_data_len_, view, view_len);
    log_info(QUERY_LOG, "get view [%s] for user\n", view);
    session->query_data_len_ += view_len;
    //append the view name len to the last byte
    session->query_raw_data_[session->query_data_len_] = view_len;
    ++session->query_data_len_;

    dns_client_send_query(session->zipper_->dns_client_,
                          session->query_raw_data_,
                          session->query_data_len_,
                          reply_to_end_user, session);
}

static void
reply_to_end_user(dns_client_error_code_t error_code,
                  const char *dns_raw_data,
                  uint16_t raw_data_len,
                  const addr_t *name_server_addr,
                  void *arg)
{
    query_session_t *session = (query_session_t *)arg;
    if (error_code != QUERY_NO_ERROR)
    {
        log_warning(QUERY_LOG, "has error during query backend named server\n");
        memorypool_free_node(session->zipper_->query_session_pool_, session);
        return;
    }
    socket_write_to(&session->client_socket_,
                    (uint8_t *)dns_raw_data,
                    raw_data_len,
                    &session->client_addr_);

   /* -----------------test for dns cache ---------------*/ 
    socket_t client_socket;
    socket_open(&client_socket, AF_INET, SOCK_DGRAM, 0);
    socket_set_unblock(&client_socket, true);
    addr_t server_addr;
    addr_init(&server_addr, dnscache_ip, dnscache_port);
    if (socket_connect(&client_socket, &server_addr) == 0)
    {
         socket_write(&client_socket,
                      (uint8_t *)dns_raw_data,
                      raw_data_len);
    }
    socket_close(&client_socket);
    /*---------------------------------------------------*/
    
    memorypool_free_node(session->zipper_->query_session_pool_, session);
}

static void
zipper_print_help()
{
    fprintf(stderr, "Usage: zipper [OPTION]...\n");
    fprintf(stderr,
            "Supported options:\n"
            "  -c configfile   specify the root config file path.\n"
            "  -f              run as foreground.\n"
            "  -h              Print this help information.\n"
            "  -v              Print version information.\n");

    fprintf(stderr, "Version %s. Report bugs to <%s>.\n",
            PACKAGE_VERSION, PACKAGE_BUGREPORT);

}

static void
zipper_print_version(void)
{
    fprintf(stderr, "%s version %s\n", PACKAGE_NAME, PACKAGE_VERSION);
    fprintf(stderr, "Written by knet.\n");
    fprintf(stderr,
            "Copyright (C) 2011 knet.\n");
    exit(0);
}

static void
zipper_init_command_server(zipper_t *zipper, const char *config_path)
{
    config_t *command_server_conf = NULL;
    int ret = conf_get_sub_config(zipper->config_,
                                  "command_server_config",
                                  &command_server_conf);
    ASSERT(ret == 0, "get configure for command server failed\n");
    char ip[MAX_IP_STR_LEN];
    int port;
    conf_get_string(command_server_conf, "ip", ip);
    conf_get_integer(command_server_conf, "port", &port);
    char command_spec_file_path[PATH_MAX];
    if (0 != conf_get_path(command_server_conf, 
                           "command_spec_file", 
                           command_spec_file_path))
        ASSERT(0, "command specific file doesn't found\n");

    zipper->command_server_ = command_server_create(zipper->base_event_, 
                                                    ip, 
                                                    port, 
                                                    command_spec_file_path,
                                                    config_path);

    conf_destroy(command_server_conf);

    command_server_init_command_runner(zipper->command_server_,
                                       zipper->rule_store_,
                                       zipper->ip_store_);
}


static void
zipper_init_config(zipper_t *zipper, const char *config_file_path)
{
    zipper->config_ = conf_create(config_file_path);
    if (zipper->config_ == NULL)
        ASSERT(0, "config file doesn't exists or has syntax error\n", 
                config_file_path);
}

static void
zipper_init_log(zipper_t *zipper)
{
    config_t *log_conf = NULL;
    int ret = conf_get_sub_config(zipper->config_, "log_config", &log_conf);
    ASSERT(ret == 0, "read log configure from configure file failed\n");
    log_init(log_conf);
    conf_destroy(log_conf);
}

static void
zipper_init_server(zipper_t *zipper)
{
    config_t *zipper_server_config = NULL;
    int ret = conf_get_sub_config(zipper->config_, 
                                  "server_config", 
                                  &zipper_server_config);
    ASSERT(ret == 0, "get configure for server failed\n");
    char ip[MAX_IP_STR_LEN];
    int port;
    conf_get_string(zipper_server_config, "ip", ip);
    conf_get_integer(zipper_server_config, "port", &port);
    zipper->dns_server_ =
        dns_server_create(zipper->base_event_, UDP_SERVER, ip, port);
    conf_destroy(zipper_server_config);

    command_server_get_named_ip(zipper->command_server_, ip);
    port = command_server_get_named_port(zipper->command_server_);

    zipper->dns_client_ =
        dns_client_create(zipper->base_event_, UDP_CLIENT, ip, port);

}

static void
zipper_init_sig(zipper_t *zipper)
{
    struct sigaction sighandler;
    sighandler.sa_handler = SIG_IGN;
    sighandler.sa_flags = 0;
    sigfillset(&sighandler.sa_mask);

    int ret = sigaction(SIGPIPE, &sighandler, NULL);
    ASSERT (ret == 0, "sigaction failed");
    ret = sigaction(SIGHUP, &sighandler, NULL);
    ASSERT (ret == 0, "sigaction failed");

    zipper->sig_int_event_ = evsignal_new(zipper->base_event_, 
                                          SIGINT, 
                                          zipper_stop_call_back, 
                                          zipper);

    zipper->sig_term_event_ = evsignal_new(zipper->base_event_, 
                                           SIGTERM, 
                                           zipper_stop_call_back, 
                                           zipper);
    event_add(zipper->sig_int_event_, NULL);
    event_add(zipper->sig_term_event_, NULL);
}

int main(int argc, char *argv[])
{
    char zipper_config_file[PATH_MAX] = "";
    bool run_foreground = false;
    strcpy(dnscache_ip, "127.0.0.1");
    dnscache_port = 5053;

    int opt = 0;
    while ((opt = getopt(argc, argv, "fvc:s:p:")) != -1)
    {
        switch(opt)
        {
            case 'c':
                strncpy(zipper_config_file, optarg, PATH_MAX);
                break;
            case 'f':
                run_foreground = true;
                break;
            case 's':
                strncpy(dnscache_ip, optarg, 16); 
                break;
            case 'p':
                dnscache_port = strtol(optarg, NULL, 10);
                break;
            case 'h':
                zipper_print_help();
                exit(0);
            case 'v':
                zipper_print_version();
                exit(0);
            default:
                zipper_print_help();
                exit(0);
        }
    }

    zipper_t *zipper = zipper_create(zipper_config_file);
    zipper_run(zipper, run_foreground);
    zipper_delete(zipper);
    return 0;
}

