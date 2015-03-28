#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "log_config.h"



config_t * config_create(const char *filename)
{
    FILE *fp = fopen(filename, "r");
    if (!fp) 
    {
        fprintf(stderr, "Error: not find config file %s\n", filename);
        return NULL;
    }
    config_t * conf = malloc(sizeof(config_t));
    memset(conf, '\0', sizeof(config_t));
    char tempconf[1024];
    while(fgets(tempconf, 1024, fp))
    {
        int len = strlen(tempconf);
        if (len <= 1)
            continue;
        strip(tempconf);
        if (tempconf[0] == '#')
            continue;
        tempconf[len - 1] = '\0';
        char *p = strchr(tempconf, '='); 
        if (p)
            *p = '\0';
        strip(tempconf);
        p++;
        strip(p);
        printf("[key=%s\t", tempconf);
        printf("value=%s]\n", p);
        if (strcmp("enable_auto", tempconf) == 0 )
           conf->enable_auto = atoi(p);
        else if (strcmp("home_dir", tempconf) == 0 )
           strcpy(conf->home_dir, p);
        else if (strcmp("file_prefix", tempconf) == 0 )
           strcpy(conf->file_prefix, p);
        else if (strcmp("file_prefix1", tempconf) == 0 )
           strcpy(conf->file_prefix1, p);
        else if (strcmp("file_prefix2", tempconf) == 0 )
           strcpy(conf->file_prefix2, p);
        else if (strcmp("file_size_limit", tempconf) == 0 )
           conf->file_size_limit = atoi(p);
        else if (strcmp("max_file_count", tempconf) == 0 )
           conf->max_file_count = atoi(p);
        else if (strcmp("rate_threshold", tempconf) == 0 )
           conf->rate_threshold = atof(p);
        else if (strcmp("important_domain_file", tempconf) == 0 )
           strcpy(conf->important_domain_file, p);
        else if (strcmp("important_network_file", tempconf) == 0 )
           strcpy(conf->important_network_file, p);
        else if (strcmp("redis_unix_sock", tempconf) == 0 )
           strcpy(conf->redis_unix_sock, p);
        else if (strcmp("rate_write_interval", tempconf) == 0 )
           conf->rate_write_interval = atoi(p);
        else if (strcmp("server_port", tempconf) == 0 )
           conf->server_port = atoi(p);
        else if (strcmp("server_mgr_port", tempconf) == 0 )
           conf->server_mgr_port = atoi(p);
        else if (strcmp("server_ip", tempconf) == 0 )
           strcpy(conf->server_ip, p);
        else if (strcmp("server_mgr_ip", tempconf) == 0 )
           strcpy(conf->server_mgr_ip, p);
        else if (strcmp("stats_map_file", tempconf) == 0 )
           strcpy(conf->stats_map_file, p);
        else if (strcmp("node_servers", tempconf) == 0 )
           strcpy(conf->node_servers, p);
        else if (strcmp("log_servers", tempconf) == 0 )
           strcpy(conf->log_servers, p);
        else if (strcmp("process_num", tempconf) == 0 )
           conf->process_num= atoi(p);
        else if (strcmp("dns_server", tempconf) == 0 )
           strcpy(conf->dns_server, p);
        else if (strcmp("bindaddress", tempconf) == 0 )
           strcpy(conf->bindaddress, p);
        else if (strcmp("role", tempconf) == 0 )
           strcpy(conf->role, p);
    }
    fclose(fp);
    return conf;
}
void config_output(config_t *conf, FILE *fp)
{   
    fprintf(fp, "important_network_file = %s\n", conf->important_network_file);
    fprintf(fp, "home_dir = %s\n", conf->home_dir);
    fprintf(fp, "enable_auto = %d\n", conf->enable_auto);
    fprintf(fp, "file_prefix = %s\n", conf->file_prefix);
    fprintf(fp, "file_size_limit = %d\n", conf->file_size_limit);
    fprintf(fp, "max_file_count = %d\n", conf->max_file_count);
    fflush(fp);
}

void config_write_file(config_t *conf, const char *filename)
{
    FILE *fp = fopen(filename, "w");
    if (fp == NULL) 
    {
        printf("open file failed\n");
        return;
    }
    config_output(conf, fp);
    fclose(fp);
}
void config_destroy(config_t *conf)
{
    free(conf);
}
char * strip(char *src)
{
    char *p = src + strlen(src) - 1;
    for (; p >= src && *p == ' '; p--);
    *(p + 1) = '\0';
    for (p=src; *p == ' '; p++);
    int i = 0;
    while(src[i] = p[i++]);
    return src;
}

stats_map_t *stats_map_create(const char *filename)
{
    FILE *fp = fopen(filename, "r");
    if (!fp) 
    {
        fprintf(stderr, "Error: not find config file %s\n", filename);
        return NULL;
    }
    stats_map_t * map = malloc(sizeof(stats_map_t));
    memset(map, '\0', sizeof(stats_map_t));
    map->len = 0;
    char tempconf[1024];
    while(fgets(tempconf, 1024, fp))
    {
        int len = strlen(tempconf);
        if (len <= 1)
            continue;
        tempconf[len - 1] = '\0';
        strip(tempconf);
        if (tempconf[0] == '#')
            continue;
        char *middle = strchr(tempconf, ' '); 
        if (middle)
            *middle = '\0';
        strip(tempconf);
        middle++;
        strip(middle);
        char *last = strchr(middle, ' ');
        if (last)
            *last = '\0';
        strip(middle);
        last++;
        strip(last);
        printf("prefix !!%s!!\n", tempconf);
        printf("middle !!%s!!\n", middle);
        printf("last   !!%s!!\n", last);
        strcpy(map->items[map->len].prefix, tempconf);
        strcpy(map->items[map->len].keyword, middle);
        map->items[map->len].index_pos = atoi(last);
        map->len++;
    }

    return map;
}
void stats_map_destroy(stats_map_t *map)
{
    free(map);
}

const char *
do_ioctl_get_ipaddress(const char *dev)
{
    struct ifreq ifr;
    int fd;
    unsigned long ip;
    struct in_addr tmp_addr;

    strcpy(ifr.ifr_name, dev);
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (ioctl(fd, SIOCGIFADDR, &ifr))
    {
        perror("ioctl error");
        return (char*)"ipunkwown";
    }
    close(fd);
    memcpy(&ip, ifr.ifr_ifru.ifru_addr.sa_data + 2, 4);
    tmp_addr.s_addr = ip;
    const char *ip_addr = inet_ntoa(tmp_addr);
    fprintf(stdout,"%s : %s\n", dev, ip_addr);

    return ip_addr;
}

int rev_find_str(int recv_len, char *str, char ch)
{
    if (!str)
        return -1;
    int i = 0;
    for (; i < recv_len; i++)
    {
        if (str[recv_len - i -1] == ch)
            return recv_len - i;
    }
    return 0;
}

