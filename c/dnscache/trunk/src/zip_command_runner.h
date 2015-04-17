#ifndef COMMAND_RUNNER_HEADER_H
#define COMMAND_RUNNER_HEADER_H
#include "dig_command_server.h"
#include "zip_ip_wblist.h"
#include "zip_domain_wblist.h"
#include "zip_record_store.h"
enum
{
    CACHE_WRITE,
    CACHE_QUERY
};

command_runner_t *
create_add_ip_wblist_runner(ip_wblist_t *ip_wblist);

command_runner_t *
create_delete_ip_wblist_runner(ip_wblist_t *ip_wblist);

command_runner_t *
create_add_domain_wblist_runner(domain_wblist_t *domain_wblist);

command_runner_t *
create_delete_domain_wblist_runner(domain_wblist_t *domain_wblist);

command_runner_t *
create_cache_pattern_runner(bool *write_or_read);

command_runner_t *
create_cache_capacity_runner(record_store_t *record_store);

#endif
