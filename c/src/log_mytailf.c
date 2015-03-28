#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdbool.h>

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#define BUFF_SIZE 4096

#define ERR_EXIT(m) \
    do \
    { \
        perror(m); \
        exit(EXIT_FAILURE); \
    } while(0)

 
FILE *openFile(const char *filePath)
{
    FILE *file = fopen(filePath, "r");
    if(file == NULL)
    {
        fprintf(stderr,"Error opening file: %s\n",filePath);
        exit(errno);
    }
    return(file);
}

int get_inode(struct stat *ptr)
{
    return (int)ptr->st_ino;
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage %s file\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char filename[1024];
    bool first = false;
    strcpy(filename, argv[1]);
    FILE *file;
again:    file = openFile(filename);
    if (false == first) //first open file, and move file pointer to file-end
    {
        fseek(file, 0, SEEK_END);
        first = true;
    }
    struct stat sbuf;
    if (lstat(argv[1], &sbuf) == -1)
        ERR_EXIT("stat error");
    int last_inode = get_inode(&sbuf);
    int ch;
goon:    ch = getc(file);
    int pos = 0;
    int count = 0;
    char strbuf[BUFF_SIZE]; 
    while (ch != EOF) 
    {
        /* display contents of file on screen */
        strbuf[pos++] = ch;
        if (ch == '\n')
        {
             strbuf[pos] = '\0';
             fprintf(stdout, "%s", strbuf);
             fflush(stdout);
             pos = 0;
        }
        ch = getc(file);
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

    fclose(file);
    sleep(5);
    
    return 0;
}


