//  Task ventilator
//  Binds PUSH socket to tcp://localhost:5557
//  Sends batch of tasks to workers via that socket

#include <stdio.h>
#include <stdlib.h>
#include "zhelpers.h"
#include "logmessage.pb-c.h"
#define MAX_MSG_SIZE 1024

int main (int argc , char *argv[]) 
{

    void *buf;                     // Buffer to store serialized data
    unsigned len;                  // Length of serialized data
    void *context = zmq_ctx_new ();

    //  Socket to send messages on
    void *sender = zmq_socket (context, ZMQ_PUSH);
    int ret = zmq_bind (sender, "udp://*:5557");
    printf("te %d\n", ret);
    char *len1 = "1000";
    //zmq_setsockopt(sender, ZMQ_SNDHWM, len1, strlen(len1));
    //zmq_setsockopt(sender, ZMQ_SNDBUF, len1, strlen(len1));
    printf ("Sending tasks to workers...\n");
    LogMessage msg = LOG_MESSAGE__INIT; // AMessage
    int i = 0;
    for(; i < 15; i++)
    {
        msg.date = 2014;
        msg.cip = "203.119.80.40";
        msg.port = 3456 + i;
        msg.view = "default";
        msg.domain="www.baidu123.com";
        msg.cls = "IN";
        msg.rtype = "A";
        msg.rcode= "NOERROR";
    //msg.a = atoi(argv[1]);
    //msg.s = argv[2];
    //if (argc == 4) { msg.has_b = 1; msg.b = atoi(argv[3]); }
    len = log_message__get_packed_size(&msg);

    buf = malloc(len);
    log_message__pack(&msg,buf);

    zmq_send(sender, buf, len, 0);
    fprintf(stderr,"Writing %d serialized bytes\n",len); // See the length of message

    free(buf); // Free the allocated serialized buffer
    }
    zmq_close (sender);
    zmq_ctx_destroy (context);
    return 0;
}
