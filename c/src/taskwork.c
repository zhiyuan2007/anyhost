//  Task worker
//  Connects PULL socket to tcp://localhost:5557
//  Collects workloads from ventilator via that socket
//  Connects PUSH socket to tcp://localhost:5558
//  Sends results to sink via that socket

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <hiredis.h>
#include <async.h>
#include <signal.h>
#include <adapters/libevent.h>
#include "zhelpers.h"
#include <pthread.h>
#include "logmessage.pb-c.h"
#include "util/log_split.h"
#include "util/log_config.h"
#include "util/dig_logger.h"
#define MAX_MSG_SIZE 1024

int run = 1;
int count = 0;
int thread_num = 0;

struct zmq_thread_data
{
    void *ctx;
    void *socket;
    config_t *conf;
    zddi_logger_t *logger;
    char server_ip[256];
    int count;
};

typedef struct zmq_thread_data zmq_thread_data_t;

zmq_thread_data_t *keeper[100];


void signal_cb(int sig)
{
    if (sig == SIGINT || sig == SIGTERM)
        run = 0;
    printf("total receive count: %d\n", count);
    int i = 0;
    for (; i <= thread_num; i++)
    {
        if (i == 0)
            config_destroy(keeper[i]->conf);
        delete_logger(keeper[i]->logger);
        zmq_close (keeper[i]->socket);
        zmq_ctx_destroy (keeper[i]->ctx);
        free(keeper[i]);
    }

}

static void *worker_proc(void *userdata){

    zmq_thread_data_t *ztd = userdata;
    ztd->ctx = zmq_ctx_new ();
    ztd->socket = zmq_socket(ztd->ctx,ZMQ_PULL);
    char url[256];
    sprintf(url, "tcp://%s:5557", ztd->server_ip);
    zmq_connect(ztd->socket, url);
    char file_prefix[256];
    config_t *conf = ztd->conf;
    sprintf(file_prefix, "%s-%s", ztd->server_ip, conf->file_prefix);
    ztd->logger = create_logger(conf->home_dir, file_prefix, conf->max_file_count, conf->file_size_limit);
    struct tm *ptm;
    char query_log[1024];
    char datetime[100];
    char buf[1024];
    int msg_len;
    time_t tt;
    LogMessage *msg;
    pthread_t _pid = pthread_self();
    while(run)
    {
         msg_len = zmq_recv(ztd->socket, buf, MAX_MSG_SIZE, 0);
         msg = log_message__unpack(NULL, msg_len, buf);   
         tt = msg->date;
         ptm = localtime(&tt);
         strftime(datetime, 100, "%Y-%m-%d %H:%M:%S ", ptm);
         sprintf(query_log, "%s %s %s: %s %d %d\n", datetime, msg->cip, msg->view, msg->domain, msg->rtype, msg->rcode);
         log_msg(ztd->logger, query_log);
         log_message__free_unpacked(msg, NULL);
         if (ztd->count++%100000== 0)
         {
             printf(query_log);
             printf("!!! thread %d:msglen %d count....%d\n", (int)_pid, msg_len, ztd->count);
         }
    }
}

int main (int argc, char *argv[]) 
{
    config_t  *conf = config_create("./split.conf");
    if (NULL == conf)
    {
        printf("create config failed, maybe file no exists\n");
        return -1;
    }
    char *peer[100];
    char dns_server[1024];
    strcpy(dns_server, conf->dns_server);
    peer[thread_num] = dns_server;
    char *pos = strchr(dns_server, ',');
    while (pos)
    {
       *pos = '\0';
       peer[++thread_num] = pos+1; 
       pos = strchr(pos+1, ',');
    }

    signal(SIGINT, signal_cb);
    signal(SIGTERM, signal_cb);

    int pid[100];
    int child ;
    int i = 0;

    for (; i <= thread_num; i++)
    {
        printf("thread num %d ip %s\n", i, peer[i]);

        if ((child = fork()) == -1)
        {
            printf("fork error\n");
            exit(2);
        }
        else if(child == 0)
        {
            pid[i] = getpid();
            zmq_thread_data_t *ztd = malloc(sizeof(zmq_thread_data_t));
            ztd->conf = conf;
            ztd->ctx = NULL;
            strcpy(ztd->server_ip, peer[i]);
            ztd->count = 0;
            ztd->logger = NULL;
            //pthread_create(&pid[i], NULL, worker_proc, ztd);
            worker_proc(ztd);
            printf("process start up\n");
            keeper[i] = ztd;
        }
        else
        {
            printf("this is pprocess\n");
        }
    }

    for ( i=0;i<=thread_num; i++)
    {
        printf("wait pid\n");
        waitpid(pid[i], NULL, 0);
    }

    return 0;
}
