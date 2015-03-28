#include <stdio.h>
#include "../zhelpers.h"
#include "../logmessage.pb-c.h"

int main()
{
        struct timeval tv1, tv2;
        void *context = zmq_ctx_new();
        if (NULL == context)
        {
            printf("aaa\n");
        }
        void *sender = zmq_socket(context, ZMQ_PUSH);
        if (NULL == sender)
        {
            printf("aaa\n");
        }
        
        unsigned int i;
        char url[256];
        sprintf(url, "tcp://*:5557");
        zmq_bind(sender, url);
        float  qps = 0.0;

        LogMessage msg = LOG_MESSAGE__INIT; // AMessage
        msg.date = 1406631891;
        msg.cip = "203.119.80.40";
        msg.view = "default";
        msg.domain = "www.baidu.com";
        msg.rtype = 1;
        msg.rcode= 3;
        int len = log_message__get_packed_size(&msg);
        void *buf = malloc(len);
        gettimeofday(&tv1, NULL);
        printf("tv second %d, us %d\n", tv1.tv_sec, tv1.tv_usec);
        log_message__pack(&msg,buf);
        for(i=0; 1; i++)
        {
            zmq_send(sender, buf, len, 0);
            if (i % 1000000 == 0)
            {
                gettimeofday(&tv2, NULL);
                double timediff = (tv2.tv_sec * 1000000 + tv2.tv_usec)- (tv1.tv_sec*1000000 + tv1.tv_usec);
                printf("total send %u\n", i);
                qps = (1000000/timediff) * 1000000;
                printf("qps %f\n", qps);
                tv1 = tv2;
            }
        }
        free(buf); // Free the allocated serialized buffer
}
