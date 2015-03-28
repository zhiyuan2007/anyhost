/*
 * log_sender.c
 * Copyright (C) 2015 liuben <liuben@ubuntu>
 *
 * Distributed under terms of the MIT license.
 */
#include "zhelpers.h"
#include <sys/stat.h>
#include <stdbool.h>
#define BUFF_SIZE 1024

void *sender = NULL;
void *context = NULL;
FILE *file = NULL;

void signal_cb(int sig)
{
	if (sender)
	    zmq_close (sender);
	if (context)
	    zmq_ctx_destroy (context);
	if (file)
		fclose(file);
	exit(0);
}

int get_inode(struct stat *ptr)
{
    return (int)ptr->st_ino;
}

int main(int argc, char **argv)
{
	if (argc != 2)
	{
		printf("usage: log_sender query.log\n");
		return 1;
	}

	char filename[256];
	strcpy(filename, argv[1]);

    void *buf;                     // Buffer to store serialized data
    unsigned len;                  // Length of serialized data
    void *context = zmq_ctx_new ();
    //  Socket to send messages on
    void *sender = zmq_socket (context, ZMQ_PUSH);
    zmq_bind (sender, "tcp://*:5557");
    signal(SIGINT, signal_cb);
    signal(SIGTERM, signal_cb);
    char line[1024];
	bool first = false;
again:    file = fopen(filename, "r");
    if (!file)
	{
        printf("query log file not exists\n");
		exit(1);
	}
    if (false == first) //first open file, and move file pointer to file-end
    {
        fseek(file, 0, SEEK_END);
        first = true;
    }
    struct stat sbuf;
    if (lstat(argv[1], &sbuf) == -1)
    {
        printf("read file stat error\n");
    }
    int last_inode = get_inode(&sbuf);
    char strbuf[BUFF_SIZE]; 
goon: while (NULL != fgets(strbuf, BUFF_SIZE, file)) 
    {
		zmq_send(sender, strbuf, strlen(strbuf)+1, 0);
    }
    if (feof(file))
    {
        lstat(argv[1], &sbuf);
        int new_inode = get_inode(&sbuf);
        if (new_inode == last_inode)
        {
            sleep(1);
            goto goon;
        }
        else
        {
            fclose(file);
            goto again;
        }
    }

    return 0;
}



