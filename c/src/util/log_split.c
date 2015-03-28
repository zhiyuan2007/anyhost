#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include "dig_logger.h"
#include "log_config.h"
#include "log_split.h"


struct logger_manager
{
    config_t *conf;
    zddi_logger_t *actual_logger[10];
    zddi_logger_t *temp;
    int size;
};

void send_to_nfile(logger_manager_t *lm, char *line_str)
{
     static int i = 0;
     log_msg(lm->actual_logger[i++%lm->size], line_str);
}

logger_manager_t *create_logger_manager(const char *filename)
{
    config_t  *conf = config_create(filename);
    if (NULL == conf)
    {
        printf("create config failed, maybe file no exists\n");
        return NULL;
    }
    printf("home %s\n", conf->home_dir);
    printf("prefix %s\n", conf->file_prefix);
    printf("file_count %d\n", conf->max_file_count);
    printf("file_size %d\n", conf->file_size_limit);
    logger_manager_t *lm  = (logger_manager_t *)malloc(sizeof(logger_manager_t));
    lm->size = 0;
    lm->conf = conf;
    //lm->temp = ((malloc(sizeof(zddi_logger_t *) * conf->process_num)));
    //lm->actual_logger = &lm->temp;
    int i;
    for ( i =0 ; i < conf->process_num; i++)
    {
        char file_prefix[256];
        sprintf(file_prefix, "%s%d", conf->file_prefix, i);
        zddi_logger_t *logger = create_logger(conf->home_dir, file_prefix, conf->max_file_count, conf->file_size_limit);
        if (NULL == logger)
        {
            printf("create logger failed, maybe file no exists\n");
            return NULL;
        }
        *(lm->actual_logger + i) = logger;
        lm->size++;
    }

    return lm;
}
void destroy_logger_manager(logger_manager_t *lm)
{
    config_destroy(lm->conf);
    int i =0 ;
    for (; i < lm->size; i++)
    {
       delete_logger(lm->actual_logger[i]);
    }
    free(lm);
}
