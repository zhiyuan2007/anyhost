#ifndef _DIG_LOG_H
#define _DIG_LOG_H

#include <stdint.h>

typedef struct zddi_logger zddi_logger_t;

zddi_logger_t *create_logger(const char *home_dir, 
                             const char *file_prefix,
                             uint16_t max_file_count,
                             uint16_t file_size_limit);

void    delete_logger(zddi_logger_t *logger);

void    log_msg(zddi_logger_t *logger, const char *message);

#endif
