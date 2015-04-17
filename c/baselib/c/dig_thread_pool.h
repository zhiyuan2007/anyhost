#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <stdint.h>
#include <stdbool.h>

//each task when it get executed, it can refer to two kinds
//of data, one is real time data which can't be determined at
//thread pool create time, so user has to specify it at each 
//time when add new task to thread.
//another is thread data, which is set at thread pool create phase
//and will be bind to each thread, they will be specified to 
//each thread
typedef struct task_parameter
{
    void *real_time_param;
    void *thread_local_data;
}task_param_t;

/*special task means task which care about which thread run it*/
typedef void *(*task_func_t)(task_param_t* parameter);

struct thread_pool;
typedef struct thread_pool thread_pool_t;

thread_pool_t*		thread_pool_create(uint32_t thread_count);
void			    thread_pool_delete(thread_pool_t *tp);
int                 thread_pool_get_thread_count(const thread_pool_t *tp);
bool                thread_pool_is_idle(thread_pool_t *tp);
void			    thread_pool_start(thread_pool_t *tp);
void 			    thread_pool_stop(thread_pool_t *tp);
void                thread_pool_force_stop(thread_pool_t *tp);

int                 thread_pool_add_task_to_run(thread_pool_t *tp, 
                                                task_func_t func, 
                                                void *task_param);


// before thread get running, set the local data to each thread
// then each task can get the thread data during execution, this 
// is useful for that each thread has specified data set.
void                thread_pool_set_thread_data(thread_pool_t *tp, 
                                                int thread_index, 
                                                void *user_data);
void *              thread_pool_get_thread_data(thread_pool_t *tp, 
                                                int thread_index);


#endif
