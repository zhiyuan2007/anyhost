//  Task worker
//  Connects PULL socket to tcp://localhost:5557
//  Collects workloads from ventilator via that socket
//  Connects PUSH socket to tcp://localhost:5558
//  Sends results to sink via that socket

#include <stdio.h>
#include <stdlib.h>
#include "zhelpers.h"
#define MAX_MSG_SIZE 1024
int main (int argc, char *argv[]) 
{
    //  Socket to receive messages on
	char *iplist[10] = {"127.0.0.1"};
    char trans[256];
    if (argc == 2)
    {
        sprintf(trans, "tcp://%s:5557", argv[1]);
    }
	else {
		printf("usage: sub [server-ip]\n");
	}
    void *context = zmq_ctx_new ();
    void *receiver = zmq_socket (context, ZMQ_PULL);
    zmq_connect (receiver,  trans);
    zmq_setsockopt(receiver, ZMQ_SUBSCRIBE, "", strlen(""));
    uint8_t buf[MAX_MSG_SIZE];

    int count = 0;
    while (1)
    {
    int msg_len = zmq_recv (receiver, buf, 255, 0);
	printf("%s\n", buf);
    }
    zmq_close (receiver);
    zmq_ctx_destroy (context);
    return 0;
}
