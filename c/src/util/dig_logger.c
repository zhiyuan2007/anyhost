#include <fcntl.h>
#include <limits.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <dirent.h>
#include <errno.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdbool.h>
#include <stdint.h>
#include "dig_buffer.h"
#include "dig_logger.h"

#define DEFAULT_FILE_BUFFER_SIZE (1 * 256)
#define MINI_BUFFER_REQ (256)

////////////////////////////////////////////////////////////////////////////////////////////////
struct zddi_logger 
{
    char *home_dir;
    char *file_prefix;
    int fd;
    uint32_t cur_file_index;
    uint32_t max_file_count;
    uint32_t file_size;
    uint32_t file_size_limit;
    char file_buffer_data[DEFAULT_FILE_BUFFER_SIZE];
    buffer_t file_buffer;
    char current_log_file_name[PATH_MAX];
};

#define LOGGER_MAX_FILE_COUNT(ml) ((ml)->max_file_count)
#define LOGGER_FILE_SIZE_LIMIT(ml) ((ml)->file_size_limit)
#define LOGGER_FILE_PREFIX(ml) ((ml)->file_prefix)
#define LOGGER_FD(ml) ((ml)->fd)
#define LOGGER_FILE_SIZE(ml) ((ml)->file_size)
#define LOGGER_HOME(ml) ((ml)->home_dir)
#define LOGGER_CURRENT_LOG_FILE_NAME(ml) ((ml)->current_log_file_name) 
#define LOGGER_BUFFER(ml) ((ml)->file_buffer)
#define LOGGER_BUFFER_DATA(ml) ((ml)->file_buffer_data)
#define LOGGER_IS_REAL_TIME(ml) ((ml)->is_real_time)
#define LOGGER_IS_FOREGROUND(ml) ((ml)->is_forgound)

static void _logger_open_log_file(zddi_logger_t* module_logger);
static inline void _logger_get_log_file_name_with_index(zddi_logger_t *module_logger, char *file_name, int index);
static void _logger_write_entry(zddi_logger_t *module_logger, const char *msg);
static void _logger_write_entry_into_buffer(zddi_logger_t *module_logger, const char *msg);
static void _logger_flush(zddi_logger_t *module_logger);
static void _logger_roll_over_written_file(zddi_logger_t *module_logger);


zddi_logger_t *
create_logger(const char *home_dir, 
              const char *file_prefix,
              uint16_t max_file_count, 
              uint16_t file_size_limit) 
{
     zddi_logger_t* module_logger = malloc(sizeof(zddi_logger_t));
     
     LOGGER_HOME(module_logger) = strdup(home_dir);
     LOGGER_FILE_PREFIX(module_logger) = strdup(file_prefix);
     LOGGER_MAX_FILE_COUNT(module_logger) = max_file_count;
     LOGGER_FILE_SIZE_LIMIT(module_logger) = file_size_limit * 1024 * 1024;
     LOGGER_FD(module_logger) = -1;

     _logger_get_log_file_name_with_index(module_logger, LOGGER_CURRENT_LOG_FILE_NAME(module_logger), -1);
     _logger_open_log_file(module_logger);
     buffer_create_from(&LOGGER_BUFFER(module_logger), 
                        LOGGER_BUFFER_DATA(module_logger), 
                        DEFAULT_FILE_BUFFER_SIZE);

     return module_logger;
}

void     
delete_logger(zddi_logger_t *logger)
{
    _logger_flush(logger);
    close(LOGGER_FD(logger));
    free(LOGGER_HOME(logger));
    free(LOGGER_FILE_PREFIX(logger));
    free(logger); 
}

void    
log_msg(zddi_logger_t *logger, const char *msg)
{
    _logger_write_entry(logger, msg);
}

static void 
_logger_open_log_file(zddi_logger_t* module_logger)
{
    const char *log_file_name = LOGGER_CURRENT_LOG_FILE_NAME(module_logger);
    LOGGER_FD(module_logger) = open(log_file_name, O_CREAT|O_APPEND|O_RDWR, 0664);
    LOGGER_FILE_SIZE(module_logger) = 0;
}

static void inline 
_logger_get_log_file_name_with_index(zddi_logger_t *module_logger, char *file_name, int index)
{
    if (index > 0)
        sprintf(file_name,"%s/%s%d", LOGGER_HOME(module_logger), LOGGER_FILE_PREFIX(module_logger), index);
    else
        sprintf(file_name,"%s/%s", LOGGER_HOME(module_logger), LOGGER_FILE_PREFIX(module_logger));
}

static void 
_logger_write_entry(zddi_logger_t *module_logger, const char *msg)
{

    if (DEFAULT_FILE_BUFFER_SIZE - buffer_position(&LOGGER_BUFFER(module_logger)) < MINI_BUFFER_REQ)
        _logger_flush(module_logger);

    _logger_write_entry_into_buffer(module_logger, msg);

    if (DEFAULT_FILE_BUFFER_SIZE - buffer_position(&LOGGER_BUFFER(module_logger)) < MINI_BUFFER_REQ)
        _logger_flush(module_logger);
    
    if (LOGGER_FILE_SIZE(module_logger) >= LOGGER_FILE_SIZE_LIMIT(module_logger))
        _logger_roll_over_written_file(module_logger);
}

static void 
_logger_flush(zddi_logger_t *module_logger)
{
    buffer_t *file_buffer = &LOGGER_BUFFER(module_logger);
    size_t buffer_data_len = buffer_position(file_buffer);
    if (buffer_data_len == 0)
        return;

    LOGGER_FILE_SIZE(module_logger) += buffer_data_len;
    write(LOGGER_FD(module_logger), buffer_data(file_buffer), buffer_data_len);
    buffer_set_position(file_buffer, 0);

}

static void 
_logger_roll_over_written_file(zddi_logger_t *module_logger)
{
    int max_file_index = LOGGER_MAX_FILE_COUNT(module_logger) - 2;/*current file is module name, first index is 0 */
    while (max_file_index > 0)
    {
        char old_file_name[PATH_MAX];
        _logger_get_log_file_name_with_index(module_logger, old_file_name, max_file_index - 1);
        struct stat file_state;
        bool file_is_valid = stat(old_file_name, &file_state) == 0;
        if (file_is_valid && S_ISREG(file_state.st_mode))
        {
            char new_file_name[PATH_MAX];
            _logger_get_log_file_name_with_index(module_logger, new_file_name, max_file_index);
            rename(old_file_name, new_file_name);
        }
        --max_file_index;
    }

    char file_with_zero_index[PATH_MAX];
    _logger_get_log_file_name_with_index(module_logger, file_with_zero_index, 0);
    close(LOGGER_FD(module_logger));
    rename(LOGGER_CURRENT_LOG_FILE_NAME(module_logger), file_with_zero_index);
    _logger_open_log_file(module_logger);
}

static void 
_logger_write_entry_into_buffer(zddi_logger_t *module_logger, const char *msg)
{
    buffer_t *file_buffer = &LOGGER_BUFFER(module_logger);
    buffer_write_string(file_buffer, msg);
}
