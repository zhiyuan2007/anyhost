//  Task ventilator
//  Binds PUSH socket to tcp://localhost:5557
//  Sends batch of tasks to workers via that socket

#include <stdio.h>
#include <stdlib.h>
#include "zhelpers.h"
#define MAX_MSG_SIZE 1024

int main (int argc , char *argv[]) 
{
    void *buf;                     // Buffer to store serialized data
    unsigned len;                  // Length of serialized data
    void *context = zmq_ctx_new ();
    //  Socket to send messages on
    void *sender = zmq_socket (context, ZMQ_PUSH);
    zmq_bind (sender, "tcp://*:5557");
	int i = 0;
	while(i < 1000000) 
	{
		char buf[1024] = "05-Dec-2014 21:02:17.588 client 127.0.0.1 38869: view default: www4130.baidu.com IN A NXDOMAIN + NS NE NT ND NC baidu.com\n";
		//zmq_setsockopt(sender, ZMQ_SNDHWM, len1, strlen(len1));
		//zmq_setsockopt(sender, ZMQ_SNDBUF, len1, strlen(len1));
		zmq_send(sender, buf, strlen(buf)+1, 0);
		i++;
	}
	zmq_close (sender);
	zmq_ctx_destroy (context);
    return 0;
}
