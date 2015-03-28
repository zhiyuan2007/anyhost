#ifndef __HEADER_LOG_SPLIT_H__
#define __HEADER_LOG_SPLIT_H__
typedef struct logger_manager logger_manager_t;
void send_to_nfile(logger_manager_t *lm, char *line_str);
logger_manager_t *create_logger_manager(const char *filename);
void destroy_logger_manager(logger_manager_t *lm);
#endif
