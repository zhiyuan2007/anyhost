#include <string.h>
#include "dig_command_server.h"
#include "zip_utils.h"
#include "zip_command_runner.h"
#include "zip_radix_tree.h"
#define MAX_IP_STR_LEN 16
#define MAX_DOMAIN_STR_LEN 256
#define MAX_PATTERN_LEN 15

#define COMMAND_GET_ARG_STRING(command, key, value) do {\
    if (0 != command_get_arg_string(command, key, value))\
    {\
        sprintf(error_message, "%s", "command parse error");\
        return 1;\
    }\
}while(0);


/*--------------------------------add ip wblist ------------------*/
static const char *
add_ip_wblist_get_name(const struct command_runner *runner)
{
    return "add_ip_wblist";
}

static int
add_ip_wblist_execute_command(struct command_runner *runner,
                                  const command_t *command,
                                  char *error_message)
{
    ip_wblist_t *ip_wblist = (ip_wblist_t *)runner->private_data_;
    char ip_mask[MAX_IP_STR_LEN];
    COMMAND_GET_ARG_STRING(command, "ip_mask", ip_mask);
    int ret = ip_wblist_insert_from_str(ip_wblist, ip_mask);
    sprintf(error_message, "%s", radix_tree_get_result(ret)); 
    return ret;
}
static void
add_ip_wblist_delete(struct command_runner *runner)
{
    free(runner);
}

command_runner_t *
create_add_ip_wblist_runner(ip_wblist_t *ip_wblist)
{
    ASSERT(ip_wblist, "ip wblist is NULL");

    command_runner_t *runner = malloc(sizeof(command_runner_t));
    if (!runner)
        return NULL;

    runner->private_data_ = ip_wblist; 
    runner->runner_get_command_name = add_ip_wblist_get_name;
    runner->runner_execute_command = add_ip_wblist_execute_command;
    runner->runner_delete = add_ip_wblist_delete;
    runner->next_ = NULL;

    return runner;
}

/*--------------------------------delete ip wblist ------------------*/

static const char *
delete_ip_wblist_get_name(const struct command_runner *runner)
{
    return "del_ip_wblist";
}

static int
delete_ip_wblist_execute_command(struct command_runner *runner,
                                  const command_t *command,
                                  char *error_message)
{
    ip_wblist_t *ip_wblist = (ip_wblist_t *)runner->private_data_;
    char ip_mask[MAX_IP_STR_LEN];
    COMMAND_GET_ARG_STRING(command, "ip_mask", ip_mask);

    int ret = ip_wblist_delete_from_str(ip_wblist, ip_mask);
    sprintf(error_message, "%s", radix_tree_get_result(ret)); 

    return ret;
}
static void
delete_ip_wblist_delete(struct command_runner *runner)
{
    free(runner);
}

command_runner_t *
create_delete_ip_wblist_runner(ip_wblist_t *ip_wblist)
{
    ASSERT(ip_wblist, "ip wblist is NULL");

    command_runner_t *runner = malloc(sizeof(command_runner_t));
    if (!runner)
        return NULL;

    runner->private_data_ = ip_wblist; 
    runner->runner_get_command_name = delete_ip_wblist_get_name;
    runner->runner_execute_command = delete_ip_wblist_execute_command;
    runner->runner_delete = delete_ip_wblist_delete;
    runner->next_ = NULL;

    return runner;
}
/*--------------------------------add domain wblist ------------------*/

static const char *
add_domain_wblist_get_name(const struct command_runner *runner)
{
    return "add_domain_wblist";
}

static int
add_domain_wblist_execute_command(struct command_runner *runner,
                                  const command_t *command,
                                  char *error_message)
{
    domain_wblist_t *domain_wblist = (domain_wblist_t *)runner->private_data_;
    char domain[MAX_DOMAIN_STR_LEN];
    
    COMMAND_GET_ARG_STRING(command, "domain_name", domain);
    int ret = domain_wblist_insert_from_text(domain_wblist, domain);
    sprintf(error_message, "%s", domain_wblist_get_result(ret)); 
    return ret;
}

static void
add_domain_wblist_delete(struct command_runner *runner)
{
    free(runner);
}

command_runner_t *
create_add_domain_wblist_runner(domain_wblist_t *domain_wblist)
{
    ASSERT(domain_wblist, "domain wblist is NULL");

    command_runner_t *runner = malloc(sizeof(command_runner_t));
    if (!runner)
        return NULL;

    runner->private_data_ = domain_wblist; 
    runner->runner_get_command_name = add_domain_wblist_get_name;
    runner->runner_execute_command = add_domain_wblist_execute_command;
    runner->runner_delete = add_domain_wblist_delete;
    runner->next_ = NULL;

    return runner;
}

/*--------------------------------delete domain wblist ------------------*/

static const char *
delete_domain_wblist_get_name(const struct command_runner *runner)
{
    return "del_domain_wblist";
}

static int
delete_domain_wblist_execute_command(struct command_runner *runner,
                                  const command_t *command,
                                  char *error_message)
{
    domain_wblist_t *domain_wblist = (domain_wblist_t *)runner->private_data_;
    char domain[MAX_DOMAIN_STR_LEN];
    COMMAND_GET_ARG_STRING(command, "domain_name", domain);
    
    int ret = domain_wblist_delete_from_text(domain_wblist, domain);
    sprintf(error_message, "%s", domain_wblist_get_result(ret)); 
    return ret;
}

static void
delete_domain_wblist_delete(struct command_runner *runner)
{
    free(runner);
}

command_runner_t *
create_delete_domain_wblist_runner(domain_wblist_t *domain_wblist)
{
    ASSERT(domain_wblist, "domain wblist is NULL");

    command_runner_t *runner = malloc(sizeof(command_runner_t));
    if (!runner)
        return NULL;

    runner->private_data_ = domain_wblist; 
    runner->runner_get_command_name = delete_domain_wblist_get_name;
    runner->runner_execute_command = delete_domain_wblist_execute_command;
    runner->runner_delete = delete_domain_wblist_delete;
    runner->next_ = NULL;

    return runner;
}

/*--------------------------------start cache pattern ------------------*/

static const char *
cache_pattern_get_name(const struct command_runner *runner)
{
    return "set_pattern";
}

static int
cache_pattern_execute_command(struct command_runner *runner,
                                  const command_t *command,
                                  char *error_message)
{
    bool *w_or_r = (bool *)runner->private_data_;
    char action[MAX_PATTERN_LEN];
    
    COMMAND_GET_ARG_STRING(command, "pattern", action);

    if (strcmp(action, "write") == 0)
        *w_or_r = CACHE_WRITE;
    else
        *w_or_r = CACHE_QUERY;

    return 0;
}

static void
cache_pattern_delete(struct command_runner *runner)
{
    free(runner);
}

command_runner_t *
create_cache_pattern_runner(bool *write_or_read)
{
    command_runner_t *runner = malloc(sizeof(command_runner_t));
    if (!runner)
        return NULL;

    runner->private_data_ = write_or_read; 
    runner->runner_get_command_name = cache_pattern_get_name;
    runner->runner_execute_command = cache_pattern_execute_command;
    runner->runner_delete = cache_pattern_delete;
    runner->next_ = NULL;

    return runner;
}

/*--------------------------------set cache capacity------------------*/

static const char *
cache_capacity_get_name(const struct command_runner *runner)
{
    return "set_capacity";
}

static int
cache_capacity_execute_command(struct command_runner *runner,
                               const command_t *command,
                               char *error_message)
{
    record_store_t *record_store = (record_store_t *)runner->private_data_;
    uint32_t  capacity;
    if (0 != command_get_arg_integer(command, "capacity", &capacity))
    {
        sprintf(error_message, "%s", "command parse error");
        return 1;
    }
    record_store_set_capacity(record_store, capacity);

    return 0;
}

static void
cache_capacity_delete(struct command_runner *runner)
{
    free(runner);
}

command_runner_t *
create_cache_capacity_runner(record_store_t *record_store)
{
    command_runner_t *runner = malloc(sizeof(command_runner_t));
    if (!runner)
        return NULL;

    runner->private_data_ = record_store; 
    runner->runner_get_command_name = cache_capacity_get_name;
    runner->runner_execute_command = cache_capacity_execute_command;
    runner->runner_delete = cache_capacity_delete;
    runner->next_ = NULL;

    return runner;
}
