#ifndef __LOG_UTILIS__H
#define __LOG_UTILIS__H
#include <stdio.h>
#define MAX(a,b) ((a)>(b)?(a):(b))
#define MIN(a,b) ((a)<(b)?(a):(b))

#define CHECK(func) \
do {\
    ret = func; \
    if (ret != SUCCESS) \
    {\
        TRACE(DIG_ERROR, #func, ret); \
        return ret;\
    } \
}while (0);

#define DIG_MALLOC(p,size) do {\
    (p) = malloc((size));\
    memset((p), 0, (size));\
}while (0)

#define DIG_REALLOC(p, new_size) do{\
    (p) = realloc((p), new_size);\
    ASSERT((p),"realloc memory failed");\
}while (0)

#define DIG_CALLOC(p, count, size) do {\
    (p) = calloc((count), (size));\
    ASSERT((p),"calloc memory failed");\
}while (0)

#define ASSERT(cond,...)    do{ \
    if (!(cond))  {\
        char str[200];\
        sprintf(str, __VA_ARGS__);\
        printf("%s in file %s at line %d\n", str, __FILE__, __LINE__);\
        exit(1);    \
    }\
}while (0)


#define REDIS_INSTANCE 100
#define SP " "
typedef struct config
{
    char filename[100];
    char eth_dev[20];
    char server_ip[20];
    int server_port;
    int server_mgr_port;
    int max_line;
    int enable_auto;
    char home_dir[128];
    char file_prefix[128];
    int max_file_count;
    int file_size_limit;
    int redis_unix_num;
    char *redis_unix_sock[REDIS_INSTANCE];
    char log_ip[100];
}config;

config * config_create(const char *filename);
void config_destroy(config *conf);
void config_output(config *conf, FILE *fp);
void config_write_file(config *conf);
char * strip(char *src);
const char *do_ioctl_get_ipaddress(const char *dev);
int rev_find_str(int recv_len, char *str, char ch);
#endif
