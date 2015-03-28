/*
 * main.c
 * Copyright (C) 2015 liuben <liuben@ubuntu>
 *
 * Distributed under terms of the MIT license.
 */

#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <time.h>
#include <stdbool.h>
#include "log_topn.h"

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        printf("usage %s query_log_file\n", argv[0]);
        return 1;
    }
    house_keeper_t *keeper = house_keeper_create();
    if (keeper)
        printf("create log server manager success\n");
    else
    {
        printf("craete log server manager failed\n");
        return 1;
    }
    char filename[1024];
    bool first = false;
    strcpy(filename, argv[1]);
    char line[1024];
    FILE *file;
again:    file = fopen(filename, "r");
    if (!file)
	{
        printf("query log file %s not exists\n", filename);
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
    char strbuf[1024]; 
goon: while (NULL != fgets(strbuf, 1024, file)) 
    {
       handle_string_log(keeper, strbuf);
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

