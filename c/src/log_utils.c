#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "log_utils.h"

config * config_create(const char *filename)
{
    config * conf = malloc(sizeof(config));
    FILE *fp = fopen(filename, "r");
    if (!fp) 
    {
        fprintf(stderr, "Error: not find config file log.conf\n");
    }
    else
    {
        strcpy(conf->filename, filename);
        conf->redis_unix_num = 0;
        char tempconf[1024];
        while(fgets(tempconf, 1024, fp))
        {
            int len = strlen(tempconf);
            if (len <= 1)
                continue;
            if (tempconf[0] == '#')
                continue;
            tempconf[len - 1] = '\0';
            strip(tempconf);
            char *p = strchr(tempconf, '='); 
            if (p)
                *p = '\0';
            strip(tempconf);
            p++;
            strip(p);
            if (strcmp("eth_dev", tempconf) == 0 )
               strcpy(conf->eth_dev, p); 
            else if (strcmp("enable_auto", tempconf) == 0 )
            {
               conf->enable_auto = atoi(p);
            }
            else if (strcmp("redis_sock_path", tempconf) == 0 )
            {
                char unix_socks[1024];
                strcpy(unix_socks, p);
                char *str1, *str2, *token;
                char *saveptr1, *saveptr2;
                for (str1= unix_socks; ;str1= NULL ) 
                {
                    token = strtok_r(str1, SP, &saveptr1);
                    if (token == NULL)
                        break;
                    int t_len = strlen(token);
                    char *oneofpath = malloc(t_len + 1);
                    strcpy(oneofpath, token);
                    conf->redis_unix_sock[conf->redis_unix_num] = oneofpath;
                    conf->redis_unix_num++;
                }
            }
            else if (strcmp("server_ip", tempconf) == 0 )
               strcpy(conf->server_ip, p);
            else if (strcmp("server_port", tempconf) == 0 )
               conf->server_port = atoi(p);
            else if (strcmp("server_mgr_port", tempconf) == 0 )
               conf->server_mgr_port = atoi(p);
            else if (strcmp("max_line", tempconf) == 0 )
               conf->max_line = atoi(p);
            else if (strcmp("home_dir", tempconf) == 0 )
               strcpy(conf->home_dir, p);
            else if (strcmp("file_prefix", tempconf) == 0 )
               strcpy(conf->file_prefix, p);
            else if (strcmp("file_size_limit", tempconf) == 0 )
               conf->file_size_limit = atoi(p);
            else if (strcmp("max_file_count", tempconf) == 0 )
               conf->max_file_count = atoi(p);
            else if (strcmp("log_ip", tempconf) == 0 )
               strcpy(conf->log_ip, p);
        }
        fclose(fp);
    }
    return conf;
}
void config_output(config *conf, FILE *fp)
{   
    fprintf(fp, "eth_dev = %s\n", conf->eth_dev);
    fprintf(fp, "server_ip = %s\n", conf->server_ip);
    fprintf(fp, "server_port = %d\n", conf->server_port);
    fprintf(fp, "server_mgr_port = %d\n", conf->server_mgr_port);
    fprintf(fp, "max_line = %d\n", conf->max_line);
    fprintf(fp, "home_dir = %s\n", conf->home_dir);
    fprintf(fp, "enable_auto = %d\n", conf->enable_auto);
    fprintf(fp, "file_prefix = %s\n", conf->file_prefix);
    fprintf(fp, "file_size_limit = %d\n", conf->file_size_limit);
    fprintf(fp, "max_file_count = %d\n", conf->max_file_count);
    fprintf(fp, "redis_sock_path = ");
    int i = 0;
    for (; i < conf->redis_unix_num; i++)
    {
        fprintf(fp, "%s ", conf->redis_unix_sock[i]);
    }
    fprintf(fp, "\n");
    fprintf(fp, "log_ip = %s\n", conf->log_ip);
    fflush(fp);
}

void config_write_file(config *conf)
{
    FILE *fp = fopen(conf->filename, "w");
    if (fp == NULL) 
    {
        printf("open file failed\n");
        return;
    }
    config_output(conf, fp);
    fclose(fp);
}
void config_destroy(config *conf)
{
    int i = 0 ;
    for (; i < conf->redis_unix_num; i++)
    {
        free(conf->redis_unix_sock[i]);
    }
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
