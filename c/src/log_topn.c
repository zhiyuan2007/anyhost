#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <assert.h>
#include <pthread.h>
#include "log_utils.h"
#include "log_topn.h"
#include "log_name_tree.h"
#include "log_view_tree.h"
#include "zip_addr.h"
#include "zip_socket.h"
#include "statsmessage.pb-c.h"
#include "adlist.h"
#define RECORD_LEN 1024
#define PART_LOG_LEN 768
#define SP_NUM 20
#define REDIS_MAX_CLIENT 100

#define BUFF_SIZE 1024



void *command_handler(void *args);
void *qps_thread(void *args);
void house_keeper_destroy(house_keeper_t *keeper);
void add_init_view(house_keeper_t *keeper, const char *name);

static house_keeper_t *keeper;

struct house_keeper {
    float qps;
    float success_rate;
    uint64_t count;
    view_tree_t *views;
    socket_t socket;
    pthread_t tid;
    pthread_t qps_tid;
	pthread_mutex_t 	mlock;
	uint64_t mem_uplimit;
};

static inline
int name_compare(const void *value1, const void *value2)
{
    assert(value1 && value2);
    const char *name1 = ((view_tree_node_t *)value1)->name;
    const char *name2 = ((view_tree_node_t *)value2)->name;

    return strcmp(name1, name2);
}

int signal_cb(int sig)
{
    if (sig ==  SIGINT || sig == SIGTERM)
    {
        house_keeper_destroy(keeper);
    }
}

void free_list_node(void *node)
{
    if (node)
        free(node);
}

int get_inode(struct stat *ptr)
{
    return (int)ptr->st_ino;
}

void log_handle(house_keeper_t *keeper, const char *view, const char *domain,
        const char *ip, const char *rtype, const char *rcode)
{

    ASSERT(keeper && view && domain && ip ,"invalid pointer\n");
	pthread_mutex_lock(&keeper->mlock);
    view_tree_node_t *vtnode = view_tree_find(keeper->views, view); 
	view_stats_t *vs = NULL;
    if (NULL == vtnode)
    {
        vs = view_stats_create(view);
        if (NULL == vs)
        {
            printf("not  enough memory\n");
	        pthread_mutex_unlock(&keeper->mlock);
            return;
        }
        view_tree_insert(keeper->views, view, vs);
		printf("view %s create stats vs %p\n", view, vs);
    }else {
		vs = vtnode->vs;
		lru_list_move_to_first(keeper->views->lrulist, vtnode->ptr); 
	}
	uint64_t current_mem_size = view_tree_get_size(keeper->views);
	if (keeper->mem_uplimit < current_mem_size) {
		printf("start to recycle memory current size %lld, uplimit size %lld\n", current_mem_size, keeper->mem_uplimit);
	    view_tree_set_memsize(keeper->views, keeper->mem_uplimit);
	}

    view_stats_insert_name(vs, domain);
    view_stats_insert_ip(vs, ip);
    view_stats_rtype_increment(vs, rtype_index(rtype));
    view_stats_rcode_increment(vs, rcode_index(rcode));
	pthread_mutex_unlock(&keeper->mlock);
}

void handle_string_log(house_keeper_t *keeper , char *line)
{
    char *str1, *str2, *saveptr2, saveptr1;
    char *ptr[100];
    int i;
    int len5;
    char query_log[RECORD_LEN];
    strcpy(query_log, line);
    for (i = 0; i< SP_NUM; i++)
        ptr[i] = NULL;
    for (i = 0 ,str2 = query_log; i < SP_NUM ; i++, str2 = NULL)
    {
        ptr[i] = strtok_r(str2, SP, &saveptr2);
        if (ptr[i] == NULL)
            break;
    }
    /*
     * 02-Jul-2013 11:36:36.273 client 203.119.80.41 57867: view interval: www14998.test.com IN A NXDOMAIN + NS NE NT ND NC
     */
    //printf("view: %s, domain: %s, rtype: %s, rcode: %s\n", ptr[VIEW_P], ptr[DOMAIN_P], ptr[RTYPE_P], ptr[RCODE_P]);
    if (ptr[VIEW_P] && ptr[CIP_P] && ptr[RCODE_P] && ptr[RTYPE_P])
    {
       if (ptr[VIEW_P][strlen(ptr[VIEW_P]) -1 ] != ':')
           return;
       ptr[VIEW_P][strlen(ptr[VIEW_P]) -1 ] = '\0';
       ptr[ZONE_P][strlen(ptr[ZONE_P]) -1 ] = '\0';
	   char keyword[256];
	   char total_keyword[256];
	   sprintf(keyword, "%s$%s", ptr[VIEW_P], ptr[ZONE_P]);
	   sprintf(total_keyword, "*$%s", ptr[ZONE_P]);
       log_handle(keeper, keyword, ptr[DOMAIN_P], ptr[CIP_P], ptr[RTYPE_P], ptr[RCODE_P]);
       log_handle(keeper, total_keyword, ptr[DOMAIN_P], ptr[CIP_P], ptr[RTYPE_P], ptr[RCODE_P]);
    }
}

void house_keeper_destroy(house_keeper_t *keeper)
{
    pthread_cancel(keeper->tid);
    pthread_cancel(keeper->qps_tid);
    socket_set_addr_reusable(&keeper->socket);
    socket_close(&keeper->socket);
    view_tree_destory(keeper->views);

	pthread_mutex_destroy(&keeper->mlock);
    free(keeper);
}

void house_keeper_clear_stats(house_keeper_t *keeper)
{
    view_tree_destory(keeper->views);
    keeper->views = view_tree_create(name_compare);
}

void add_init_view(house_keeper_t *keeper, const char *name)
{
    view_stats_t *vs = view_stats_create(name);
    ASSERT(vs, "create view %s stats failed\n", name);
    int ret = view_tree_insert(keeper->views, name, vs);
    ASSERT(ret == 0, "insert view %s node failed\n", name);
}


house_keeper_t *house_keeper_create()
{
    house_keeper_t *keeper = malloc(sizeof(house_keeper_t));
    ASSERT(keeper, "create house keeper failed\n");
    keeper->count = 0;
    keeper->qps = 0.0;
    keeper->success_rate = 1.0;
	keeper->mem_uplimit = 10 * 1024 * 1024;
    keeper->views = view_tree_create(name_compare);

    socket_open(&keeper->socket, AF_INET, SOCK_STREAM, 0);
    addr_t addr;
    addr_init(&addr, "127.0.0.1", 8999);
    socket_set_addr_reusable(&keeper->socket);
    int ret = socket_bind(&keeper->socket, &addr);
    ASSERT(ret == 0, "socket bind port 8999 failed\n");

    ret = pthread_create(&keeper->tid, NULL, command_handler, keeper);
    ASSERT(ret == 0, "create command handler thread failed\n");

    ret = pthread_create(&keeper->qps_tid, NULL, qps_thread, keeper);
    ASSERT(ret == 0, "create command handler thread failed\n");
	pthread_mutex_init(&keeper->mlock,NULL);

    return keeper;
}

unsigned int return_all_views(house_keeper_t *keeper,  char **buff)
{
    int n = keeper->views->count;
    StatsReply reply = STATS_REPLY__INIT;
    reply.key = "views";
    char **pdata = malloc(sizeof (char *) * n);
      
    rbnode_t *node;
    int i = 0;
    RBTREE_FOR(node, rbnode_t *, keeper->views->rbtree)
    {
       view_tree_node_t *tnode = (view_tree_node_t *)(node->value);
       
       if (strlen(tnode->name) == 0 )
           continue;
       pdata[i] = tnode->name;
       i++;
    }
    reply.n_name = i;
    reply.name = pdata;
    unsigned int len = stats_reply__get_packed_size(&reply);
    char *result_ptr = malloc(sizeof(char) *len);
    stats_reply__pack(&reply, result_ptr);
    *buff = result_ptr;
    free(pdata);
    return len;
}

unsigned int view_not_found(house_keeper_t *keeper, char **buff)
{
    StatsReply reply = STATS_REPLY__INIT;
    reply.key = "error";
    reply.maybe = "view not find";
    unsigned int len = stats_reply__get_packed_size(&reply);
    char *result_ptr = malloc(sizeof(char) *len);
    stats_reply__pack(&reply, result_ptr);
    *buff = result_ptr;
    return len;
}


unsigned int flush_stats(house_keeper_t *keeper, char **buff)
{
    house_keeper_clear_stats(keeper);
    StatsReply reply = STATS_REPLY__INIT;
    reply.key = "flush";
    reply.maybe = "success";
    unsigned int len = stats_reply__get_packed_size(&reply);
    char *result_ptr = malloc(sizeof(char) *len);
    stats_reply__pack(&reply, result_ptr);
    *buff = result_ptr;
    return len;
}

uint32_t absolute_diff(uint32_t first, uint32_t second)
{
    if (second >= first)
        return second - first;
    else
        return UINT32_MAX - first + second;
}
void *qps_thread(void *args)
{
    house_keeper_t *keeper = (house_keeper_t *)args;
    rbnode_t *node;

    int timediff = 10;
    int clear_count = 0;
    while (1)
    {
        pthread_mutex_lock(&keeper->mlock);
        int i = 0;
        RBTREE_FOR(node, rbnode_t *, keeper->views->rbtree)
        {
           view_tree_node_t *tnode = (view_tree_node_t *)(node->value);
           
           if (strlen(tnode->name) == 0 )
               continue;
           view_stats_t *vs = tnode->vs;
           vs->qps = absolute_diff(vs->last_count, vs->count)*1.0 / timediff;
           vs->last_count = vs->count;
           if (vs->count == 0)
               vs->success_rate  = 1.0;
           else
               vs->success_rate = absolute_diff(vs->rcode[stats_rcode_servfail], vs->count) * 1.0/ vs->count;
        }

        pthread_mutex_unlock(&keeper->mlock);
        sleep(timediff);
    }
}

typedef struct socket_thd_data {
    socket_t *sock;
    house_keeper_t *keeper;
}socket_thd_data_t;

void *socket_thread(void *args)
{
    socket_thd_data_t *std = (socket_thd_data_t *)args;
    house_keeper_t *keeper = std->keeper;
    socket_t *pending_socket = std->sock;
    StatsRequest *request;
    while (1)
    {
        char temp[10];
        if (socket_read(pending_socket, temp, 4) <= 0)
            break;
        unsigned int prefix_len = ntohl(*((int *)&temp));
        printf("read command prefix length was %d\n", prefix_len);
        
        char *buffer = malloc(sizeof(char ) *prefix_len);
        unsigned int len = socket_read(pending_socket, buffer, prefix_len);
        request = stats_request__unpack(NULL, len, buffer);
        printf("command==> key:%s, view:%s, topn:%d\n", request->key, request->view, request->topn);
        fflush(stdout);
        char *result_ptr = NULL;
        int topn = request->topn;
        if (topn == 0)
            topn = 1000;
        int rtn_msg_len = 0;
        pthread_mutex_lock(&keeper->mlock);
        view_tree_node_t *vtnode = view_tree_find(keeper->views, request->view); 
        if (vtnode == NULL)
        {
           if (strcmp(request->key, "views") == 0)
           {
               rtn_msg_len = return_all_views(keeper, &result_ptr);
		   }
           else if (strcmp(request->key, "flush") == 0)
           {
               rtn_msg_len = flush_stats(keeper, &result_ptr);
		   }
		   else
		   {
               rtn_msg_len = view_not_found(keeper, &result_ptr);
		   }
        }
        else
        {
	       view_stats_t *vs = vtnode->vs;
		   ASSERT(vs, "view %s in view tree, but has not corresponding view stats", request->view);
           if (strcmp(request->key, "domaintopn") == 0)
           {
			   printf("5555555555555 vs %p\n", vs);
               rtn_msg_len = view_stats_name_topn(vs, topn, &result_ptr);
           }else if (strcmp(request->key, "iptopn") == 0)
           {
               rtn_msg_len = view_stats_ip_topn(vs, topn, &result_ptr);
           }else if (strcmp(request->key, "rcode") == 0)
           {
               rtn_msg_len  = view_stats_get_rcode(vs, &result_ptr);
           }else if (strcmp(request->key, "rtype") == 0)
           {
               rtn_msg_len = view_stats_get_rtype(vs, &result_ptr);
           }else if (strcmp(request->key, "qps") == 0)
           {
               rtn_msg_len = view_stats_get_qps(vs, &result_ptr);
           }else if (strcmp(request->key, "success_rate") == 0)
           {
               rtn_msg_len = view_stats_get_success_rate(vs, &result_ptr);
           }
           else if (strcmp(request->key, "flush") == 0)
           {
               rtn_msg_len = flush_stats(keeper, &result_ptr);
           }
        }
        pthread_mutex_unlock(&keeper->mlock);
        prefix_len = htonl(rtn_msg_len);
        printf("return total len: %d\n", rtn_msg_len);
        fflush(stdout);
        
        stats_request__free_unpacked(request, NULL);
        socket_write(pending_socket, (char *)&prefix_len, 4);
        socket_write(pending_socket, result_ptr, rtn_msg_len);
        free(result_ptr);
        free(buffer);
    }
    free(std);
    socket_close(pending_socket);
    printf("socket closed\n");
}

void *command_handler(void *args)
{
    house_keeper_t *keeper = (house_keeper_t *)args;
    socket_listen(&keeper->socket, 5);
    while (1)
    {
        socket_t *pending_socket =  socket_accept(&keeper->socket);
        addr_t tempaddr;
        socket_get_peer_addr(pending_socket, &tempaddr);
        char clientip[128];
        addr_get_ip(&tempaddr, clientip);
        printf("accept socket ip: %s, port: %d\n", clientip, addr_get_port(&tempaddr));
        fflush(stdout);
        socket_thd_data_t *std = malloc(sizeof(socket_thd_data_t));
        std->keeper= keeper;
        std->sock = pending_socket;
        pthread_t pid;
        pthread_create(&pid, NULL, socket_thread, std);
    }
}

