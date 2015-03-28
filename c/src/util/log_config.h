#ifndef __LOG_CONFIG__H
#define __LOG_CONFIG__H

struct config {
    int enable_auto;
    int max_file_count;
    int file_size_limit;
    int rate_write_interval;
    double rate_threshold;
    char home_dir[256];
    char file_prefix[256];
    char file_prefix1[256];
    char file_prefix2[256];
    int server_port;
    int server_mgr_port;
    char server_ip[256];
    char server_mgr_ip[256];
    char important_network_file[256];
    char important_domain_file[256];
    char redis_unix_sock[256];
    char stats_map_file[256];
    int process_num;
    char dns_server[1024];
	int node_servers_num;
	char node_servers[1024];
	int log_servers_num;
	char log_servers[1024];
	char bindaddress[32];
	char role[32];
};
typedef struct config config_t;

struct stats_map_item {
    char prefix[128];
    char keyword[128];
    int index_pos;
};

struct stats_map {
    struct stats_map_item items[100];
    int len;
};

typedef struct stats_map stats_map_t;

config_t * config_create(const char *filename);
void config_destroy(config_t *conf);
void config_output(config_t *conf, FILE *fp);
void config_write_file(config_t *conf, const char *filename);
char * strip(char *src);
const char *do_ioctl_get_ipaddress(const char *dev);
int rev_find_str(int recv_len, char *str, char ch);

stats_map_t *stats_map_create(const char *file);
void stats_map_destroy(stats_map_t *map);
#endif
