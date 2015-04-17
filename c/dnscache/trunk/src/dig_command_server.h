#ifndef ZIP_COMMAND_SERVER_H
#define ZIP_COMMAND_SERVER_H

#include <event2/event.h>
#include <stdint.h>
#include <dig_command.h>
/*
 *  As a tcp server, accept command in json format
 *  parse the command, then send to different module
 * */
#define COMMAND_ERROR_MSG_MAX_LEN 128
typedef struct command_runner
{
    const char *(*runner_get_command_name)(const struct command_runner *);
    int (*runner_execute_command)(struct command_runner *runner,
                                  const command_t *command,
                                  char *error_message);
    void (*runner_delete)(struct command_runner *runner);
    void *private_data_;
    struct command_runner *next_;
}command_runner_t;

typedef struct command_server command_server_t;

command_server_t *
command_server_create(struct event_base *base_event, 
                      const char *ip, 
                      uint16_t port, 
                      const char *command_spec_file);

void
command_server_delete(command_server_t *server);

int 
command_server_start(command_server_t *server);

int
command_server_stop(command_server_t *server);

void
command_server_add_command_runner(command_server_t *server,
                                  command_runner_t *runner);

void
command_server_delete_all_command_runner(command_server_t *server);

#endif
