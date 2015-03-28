//  Task worker
//  Connects PULL socket to tcp://localhost:5557
//  Collects workloads from ventilator via that socket
//  Connects PUSH socket to tcp://localhost:5558
//  Sends results to sink via that socket

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include <pthread.h>
#include "zhelpers.h"
#include "util/log_split.h"
#include "util/log_config.h"
#include "util/dig_logger.h"
#define MAX_MSG_SIZE 1024

pthread_mutex_t mutex ;
int run = 1;
int count = 0;

typedef struct sender_socket
{
	void *ctx;
	void *socket;
	pthread_mutex_t mutex;
	char url[64];
}sender_socket_t;

struct zmq_thread_data
{
    void *ctx;
    void *socket;
    config_t *conf;
    zddi_logger_t *logger;
    char server_ip[256];
    int count;
	sender_socket_t **senders;
};

typedef struct zmq_thread_data zmq_thread_data_t;
sender_socket_t **init_sender_socket(config_t *conf);
void delete_senders(sender_socket_t **ss, int num) ;
int get_logservers_num (const char *log_server) ;

#define THREAD_NUM 2
int server_num = 0;

zmq_thread_data_t *keeper[THREAD_NUM] = {NULL};
pthread_t tid[THREAD_NUM];

void signal_cb(int sig)
{
    if (sig == SIGINT || sig == SIGTERM)
        run = 0;
    printf("total receive count: %d\n", count);
	int i = 0;
	for (; i< server_num; i++) 
	{
		pthread_cancel(tid[i]);
		if (i == 0)
		{
            config_destroy(keeper[i]->conf);
            delete_logger(keeper[i]->logger);
			if (keeper[i]->conf->log_servers_num > 1) {
			    delete_senders(keeper[i]->senders, keeper[i]->conf->log_servers_num);
			}
		}

        zmq_close (keeper[i]->socket);
        zmq_ctx_destroy (keeper[i]->ctx);
        free(keeper[i]);
	}
	pthread_mutex_destroy(&mutex);
}

void signal_cb_slave(int sig)
{
   if (sig == SIGINT || sig == SIGTERM)
        run = 0;
    printf("total receive count: %d\n", count);
	config_destroy(keeper[0]->conf);
	delete_logger(keeper[0]->logger);

	zmq_close (keeper[0]->socket);
	zmq_ctx_destroy (keeper[0]->ctx);
	free(keeper[0]);
}

void delete_senders(sender_socket_t **ss, int num) {
	int i = 0;
	for (; i < num ; i++) {
		zmq_close(ss[i]->socket);
		zmq_ctx_destroy(ss[i]->ctx);
		pthread_mutex_destroy(&ss[i]->mutex);
		free(ss[i]);
	}
	free(ss);
}

static void *worker_proc(void *userdata){

    zmq_thread_data_t *ztd = userdata;
    ztd->ctx = zmq_ctx_new ();
    ztd->socket = zmq_socket(ztd->ctx,ZMQ_PULL);
	printf("url connect: %s\n", ztd->server_ip);
    zmq_connect(ztd->socket, ztd->server_ip);
    char buf[1024];
    char zone[128];
    pthread_t _pid = pthread_self();
    while(run)
    {
         int msg_len = zmq_recv(ztd->socket, buf, MAX_MSG_SIZE, 0);
		 if (ztd->conf->enable_auto ) {
	         pthread_mutex_lock(&mutex);
             log_msg(ztd->logger, buf);
	         pthread_mutex_unlock(&mutex);
		 }
		 if (ztd->conf->log_servers_num > 1 ) {
			 memset(zone, 0, sizeof(zone));
		     char *last_blankspace_pos = strrchr(buf, ' ');
		     if (last_blankspace_pos != NULL) {
		         strcpy(zone, last_blankspace_pos + 1);
				 zone[strlen(zone) - 1] = '\0';
		         printf("zone:!!%s!!\n", zone);
			     int len = strlen(zone); 
			     int choosed = len % ztd->conf->log_servers_num ;
			     printf("choosed %d\n", choosed);
			     pthread_mutex_lock(&ztd->senders[choosed]->mutex);
				 printf("send msg: %s, len: %d", buf, msg_len);
				 printf("send msg to %s\n", ztd->senders[choosed]->url);
			     zmq_send(ztd->senders[choosed]->socket, buf, msg_len, 0);
			     pthread_mutex_unlock(&ztd->senders[choosed]->mutex);
		     }
		 }
         if (ztd->count++%100000== 0)
         {
             printf("!!! thread %d:msglen %d count....%d\n", (int)_pid, msg_len, ztd->count);
         }
    }
}

void start_master(config_t *conf, zddi_logger_t *logger) {
	pthread_mutex_init(&mutex,NULL); 

    signal(SIGINT, signal_cb);
    signal(SIGTERM, signal_cb);

	char *temp_str  = strtok(conf->node_servers, ",");
    char *iplist[THREAD_NUM] = {NULL};
	while (temp_str)
	{
		strip(temp_str);
		iplist[server_num] = temp_str;
		temp_str = strtok(NULL, ",");
		server_num++;
	}
    conf->node_servers_num = server_num;	
	int i = 0 ;
	for (;i < server_num; i++) 
	{
        keeper[i] = malloc(sizeof(zmq_thread_data_t));
        keeper[i]->conf = conf;
        keeper[i]->ctx = NULL;
        strcpy(keeper[i]->server_ip, iplist[i]);
        keeper[i]->count = 0;
        keeper[i]->logger = logger;
		if (i == 0 && conf->log_servers_num > 1) {
		    keeper[i]->senders = init_sender_socket(conf);
		}else if (i > 0 && conf->log_servers_num > 1 ) {
		    keeper[i]->senders = keeper[0]->senders; 
		}
	    pthread_create(&tid[i], NULL, worker_proc, keeper[i]);
	}
	for (i = 0; i < THREAD_NUM; i++)
	{
	    pthread_join(tid[i], NULL);
	}

}

void start_slave(config_t *conf, zddi_logger_t *logger) {
    signal(SIGINT, signal_cb_slave);
    signal(SIGTERM, signal_cb_slave);
    zmq_thread_data_t *ztd = malloc(sizeof(zmq_thread_data_t));
    ztd->ctx = zmq_ctx_new ();
    ztd->socket = zmq_socket(ztd->ctx,ZMQ_PULL);
	ztd->conf = conf;
	ztd->logger = logger;
	keeper[0] = ztd;
	server_num = 1;
	printf("bind url: %s\n", conf->bindaddress);
    zmq_bind(ztd->socket, conf->bindaddress);
    char buf[1024];
    while(run)
    {
         int msg_len = zmq_recv(ztd->socket, buf, MAX_MSG_SIZE, 0);
		 if (ztd->conf->enable_auto ) {
             log_msg(ztd->logger, buf);
		 }
         if (ztd->count++%100000== 0)
         {
             printf("!!:msglen %d count....%d\n", msg_len, ztd->count);
         }
    }
}
 
void start_receive_log_and_store(config_t *conf)
{
    zddi_logger_t *logger = create_logger(conf->home_dir, conf->file_prefix, conf->max_file_count, conf->file_size_limit);
	if (logger == NULL)
	{
		printf("create logger failed\n");
		return;
	}

	if (strcmp(conf->role, "master") == 0 ) {
	    conf->log_servers_num = get_logservers_num(conf->log_servers);
		start_master(conf, logger);
	}else if (strcmp(conf->role, "slave") == 0) {
		start_slave(conf, logger);
	}else {
	   printf("not support this role %s\n", conf->role);
	}
}
sender_socket_t **init_sender_socket(config_t *conf) {

	sender_socket_t **ss = malloc(sizeof(sender_socket_t*) * conf->log_servers_num);
	if (ss == NULL) {
		printf("not enough momory for create sender socket array\n");
		return NULL;
	}
	char *temp_str  = strtok(conf->log_servers, ",");
	int node_num = 0;
	while (temp_str)
	{
	    sender_socket_t *sender = malloc(sizeof(sender_socket_t));
        sender->ctx = zmq_ctx_new ();
        sender->socket = zmq_socket(sender->ctx,ZMQ_PUSH);
	    pthread_mutex_init(&sender->mutex, NULL);
		strcpy(sender->url, temp_str);
	    printf("[socket senders] prepare push msg to %s\n", sender->url);
        zmq_connect(sender->socket, sender->url);
		ss[node_num] = sender;
		temp_str = strtok(NULL, ",");
		node_num++;
	}
	return ss;
}

int get_logservers_num (const char *log_server) {
	char temp_buffer[512];
	strcpy(temp_buffer, log_server);
	char *temp_str  = strtok(temp_buffer, ",");
	int node_num = 0;
	while (temp_str)
	{
		temp_str = strtok(NULL, ",");
		node_num++;
	}
	return node_num;
}

int main (int argc, char *argv[]) 
{
    if (argc != 2)
    {
        printf("usage: log_receiver config_file logserver_list\n");
        return 1;
    }

    char filename[512];
    sprintf(filename, "%s", argv[1]);
    config_t  *conf = config_create(filename);
    if (NULL == conf)
    {
        printf("create config failed, maybe file no exists\n");
        return -1;
    }

    start_receive_log_and_store(conf);
    return 0;
}
