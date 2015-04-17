#include <stdlib.h>
#include <event2/event_struct.h>
#include <dig_command_server.h>
#include <zip_socket.h>
#include <dig_mem_pool.h>
#include <zip_addr.h>
#include <zip_utils.h>


#define MAX_CONCURRENT_CLIENT 5
#define MAX_COMMAND_STRING_LEN 0xffff


struct command_server
{
    struct event_base *base_event_;
    struct event *listen_event_;
    addr_t server_addr_;
    socket_t listen_socket_;

    command_manager_t *command_manager_;
    int connect_client_count_;

    command_runner_t *command_runner_header_;
    mem_pool_t *command_session_pool_;
};

typedef struct command_session
{
    socket_t *client_socket_;
    command_server_t *command_server_;
    struct event *command_read_event_;
    char command_buffer_[MAX_COMMAND_STRING_LEN];
    size_t read_command_len_;

    struct event *command_reply_event_;
    char command_error_info_[COMMAND_ERROR_MSG_MAX_LEN];
    int command_ret_value_;
}command_session_t;

static void 
_handle_accept(evutil_socket_t listener, short event, void *arg);
static void
_handle_command(evutil_socket_t fd, short events, void *arg);
static void
_handle_reply(evutil_socket_t fd, short events, void *arg);
static int 
command_server_run_command(command_session_t *session);
void 
command_reply_client(command_session_t *session, int ret_value, const char *error_info);

static command_session_t *
command_session_create(command_server_t *server, socket_t *client_socket);
void
command_session_delete(command_session_t *session);

command_server_t *
command_server_create(struct event_base *base_event, 
                      const char *ip, 
                      uint16_t port, 
                      const char *command_spec_file)
{
    command_server_t *server
        = (command_server_t *)calloc(1, sizeof(command_server_t));

    if (server)
    {
        server->base_event_ = base_event;
        addr_init(&server->server_addr_, ip, port);
        server->command_manager_ = command_manager_create(command_spec_file);
        ASSERT(server->command_manager_, "command manager create failed\n");
        server->command_session_pool_ = mem_pool_create(sizeof(command_session_t), 0,true);
        server->command_runner_header_ = NULL;
    }

    return server;
}

void 
command_server_delete(command_server_t *server)
{
    if (server->listen_event_)
    {
        event_del(server->listen_event_);
        event_free(server->listen_event_);
    }
    command_manager_delete(server->command_manager_);
    command_server_delete_all_command_runner(server);
    mem_pool_delete(server->command_session_pool_);
    free(server);
}

int
command_server_start(command_server_t *server)
{
    socket_t *listen_socket = &server->listen_socket_;
    int socket_type = addr_get_type(&server->server_addr_) == ADDR_IPV4 ?
                    AF_INET : AF_INET6;
    socket_open(listen_socket, socket_type, SOCK_STREAM, 0);
    socket_set_unblock(listen_socket, true);
    socket_set_addr_reusable(listen_socket);
    
    if (socket_bind(listen_socket, &server->server_addr_) != 0)   
        return 1;

    if (socket_listen(listen_socket, MAX_CONCURRENT_CLIENT) != 0)
        return 1;

    server->listen_event_ = event_new(server->base_event_, 
                                      socket_get_fd(listen_socket), 
                                      EV_READ|EV_PERSIST, 
                                      _handle_accept,
                                      (void*)server);
    event_add(server->listen_event_, NULL);
    
    return 0;
}

static void
_handle_accept(evutil_socket_t listener, short event, void *arg)
{
    command_server_t *server = (command_server_t *)arg;
    socket_t *new_socket = socket_accept(&server->listen_socket_);
    if (socket_get_fd(new_socket) < 0)
    {
        free(new_socket);
    } 
    else 
    {
        if (server->connect_client_count_ == MAX_CONCURRENT_CLIENT)
        {
            socket_close(new_socket);
            free(new_socket);
            return;
        }

        server->connect_client_count_ += 1;
        socket_set_unblock(new_socket, true);
        command_session_create(server, new_socket);
    }
}

static command_session_t *
command_session_create(command_server_t *server, socket_t *client_socket)
{
    command_session_t *session
                      = mem_pool_alloc(server->command_session_pool_);
    if (session)
    {
        session->client_socket_ = client_socket;
        session->command_server_ = server;
        session->read_command_len_ = 0;
        session->command_read_event_ = event_new(server->base_event_, 
                                      socket_get_fd(client_socket), 
                                      EV_READ|EV_PERSIST, 
                                      _handle_command,
                                      (void*)session);
        event_add(session->command_read_event_, NULL);
    }

    return session;
}

void
command_session_delete(command_session_t *session)
{
    session->command_server_->connect_client_count_ -= 1;
    event_free(session->command_read_event_);
    socket_close(session->client_socket_);
    free(session->client_socket_);
    mem_pool_free(session->command_server_->command_session_pool_, session);
}

static void
_handle_command(evutil_socket_t fd, short events, void *arg)
{
    command_session_t *session = (command_session_t *)arg;
    if (session->read_command_len_ == 0)
    {
        uint16_t command_len = 0;
        int read_len = socket_read(session->client_socket_, &command_len, 2);
        if (read_len != 2)
        {
            command_session_delete(session);
            return;
        }

        command_len = ntohs(command_len);
        if (command_len == 0 || command_len > MAX_COMMAND_STRING_LEN - 2)
        {
            command_session_delete(session);
            return;
        }

        session->read_command_len_ = command_len;
    }
    else
    {
        int read_len = socket_read(session->client_socket_, 
                               session->command_buffer_, 
                               session->read_command_len_);
        if (read_len < session->read_command_len_)
        {
            command_session_delete(session);
            return;
        }

        //add the last '0', if the string isn't c-style string
        if (session->command_buffer_[read_len - 1] != 0)
        {
            session->command_buffer_[read_len] = 0;
            ++read_len;
        }

        command_server_run_command(session);
        session->read_command_len_ = 0;
    }
}

static int 
command_server_run_command(command_session_t *session)
{
    command_server_t *server = session->command_server_;
    command_t *command = command_manager_create_command
                        (server->command_manager_,
                         session->command_buffer_);
    if (command == NULL)
    {
        command_reply_client(session, 1, "command format isn't correct");
        return 1;
    }


    const char *command_name = command_get_name(command);
    int i = 0;
    command_runner_t *runner = server->command_runner_header_;
    while (runner)
    {
        command_runner_t *next = runner->next_;
        if (strcmp(command_name, 
                   runner->runner_get_command_name(runner)) == 0)
            break;
        runner = next;
    }

    int ret = 0;
    if (runner == NULL)
    {
        command_reply_client(session, 1, "has no runner");
        ret = 1;
    }
    else
    {
        char error_info[COMMAND_ERROR_MSG_MAX_LEN];
        ret = runner->runner_execute_command(runner, command, error_info);
        command_reply_client(session, ret, error_info);
    }
    command_manager_delete_command(server->command_manager_, command);
    return ret;
}

void 
command_reply_client(command_session_t *session, int ret_value, const char *error_info)
{
    struct event *write_event = 
                    event_new(session->command_server_->base_event_, 
                              socket_get_fd(session->client_socket_), 
                              EV_WRITE,
                              _handle_reply,
                              (void*)session);
    session->command_reply_event_ = write_event;
    session->command_ret_value_ = ret_value;
    if (ret_value != 0)
    {
        strncpy(session->command_error_info_, 
                error_info, 
                COMMAND_ERROR_MSG_MAX_LEN);
    } 
    event_add(session->command_reply_event_, NULL);
}

static void
_handle_reply(evutil_socket_t fd, short events, void *arg)
{
    command_session_t *session = (command_session_t *)arg;
    event_del(session->command_reply_event_);
    event_free(session->command_reply_event_);

    uint8_t ret_json[COMMAND_ERROR_MSG_MAX_LEN + 2] = "{'result':[%s]}";
    int ret_json_len = 0;
    if (session->command_ret_value_ == 0)
        ret_json_len = snprintf(ret_json + 2, 
                                COMMAND_ERROR_MSG_MAX_LEN,
                                "{\"result\":[\"%s\"]}", 
                                "succeed");
    else
        ret_json_len = snprintf(ret_json + 2, 
                                COMMAND_ERROR_MSG_MAX_LEN,
                                "{\"result\":[\"%s\",\"%s\"]}",
                                "failed", 
                                session->command_error_info_);
    *(uint16_t*)ret_json = htons(ret_json_len);
    socket_write(session->client_socket_, ret_json, ret_json_len + 2 + 1);
}

int
command_server_stop(command_server_t *server)
{
    socket_close(&server->listen_socket_);
    if (server->listen_event_)
    {
        event_del(server->listen_event_);
        event_free(server->listen_event_);
        server->listen_event_ = NULL;
        return 0;
    }
    return 1;
}

void
command_server_add_command_runner(command_server_t *server,
                                  command_runner_t *runner)
{
    runner->next_ = server->command_runner_header_;
    server->command_runner_header_ = runner;
}

void
command_server_delete_all_command_runner(command_server_t *server)
{
    command_runner_t *runner = server->command_runner_header_;
    while (runner)
    {
        command_runner_t *next = runner->next_;
        runner->runner_delete(runner);
        runner = next;
    }
}
