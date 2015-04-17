#ifndef ZIP_COMMAND_H
#define ZIP_COMMAND_H


#define MAX_COMMAND_NAME_LEN 128
#define MAX_COMMAND_VALUE_LEN 0xffff

typedef struct command_manager command_manager_t;
typedef struct command command_t;


command_manager_t * 
command_manager_create(const char *command_spec_file);
void 
command_manager_delete(command_manager_t *command_manager);

command_t *
command_manager_create_command(command_manager_t *mgr, 
                               const char *command_json);
void 
command_manager_delete_command(command_manager_t *mgr, 
                               command_t *command);



const char *
command_get_name(const command_t *command);

int
command_get_arg_string(const command_t *command,
                       const char *arg_name,
                       char *value);

int
command_get_arg_integer(const command_t *command,
                        const char *arg_name,
                        int *value);
#endif
