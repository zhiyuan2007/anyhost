#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include <dig_thread_pool.h>
#include <dig_mem_pool.h>
#include <dig_ring.h>
#include <zip_utils.h>

typedef struct task
{
	RING_ENTRY(task) 	link;
	task_func_t         func;
    task_param_t        param;
}task_t;

typedef enum
{
    IDLE,
    RUNNING,
    DONE
}thread_state_t;

struct thread_pool;
RING_HEAD(task_list, task);
typedef struct task_list task_list_t;

typedef struct thread
{
	thread_pool_t 		*manager;
	thread_state_t 		state;
	pthread_t 		    tid;

	task_list_t 		tasks;
	pthread_mutex_t 	task_list_lock;
    pthread_cond_t      task_list_aval_cond;
	mem_pool_t		    *mem_pool;

    void                *thread_local_data;
}thread_t;

struct thread_pool
{
	thread_t 		    *threads;
	uint32_t 		    thread_count;
	uint32_t		    rb;
	pthread_mutex_t		rb_lock;
};


#define TASK_POOL_SIZE 1024

static int add_task_to_thread(thread_t *t, task_func_t func, void *param);
static int get_next_thread_to_run(thread_pool_t *tp);

static void init_thread(thread_pool_t *tp, thread_t *t);
static void start_thread(thread_t *t);
static void stop_thread(thread_t *t);
static void force_stop_thread(thread_t *t);
static void *thread_func(void *param);



thread_pool_t * 
thread_pool_create(uint32_t thread_count)
{
    thread_pool_t *tp = NULL;
    DIG_CALLOC(tp, 1, sizeof(thread_pool_t));
	if (!tp)
		return NULL;
    DIG_MALLOC(tp->threads, thread_count * sizeof(thread_t));
	int i = 0;
	for (; i < thread_count; ++i)
		init_thread(tp, tp->threads + i);
	tp->thread_count = thread_count;
	pthread_mutex_init(&tp->rb_lock, NULL);
	return tp;
}

void 
thread_pool_delete(thread_pool_t *tp)
{
    ASSERT(tp, "delete empty thread pool");
	pthread_mutex_destroy(&tp->rb_lock);
	free(tp->threads);
	free(tp);
}

int thread_pool_get_thread_count(const thread_pool_t *pool)
{
    return pool->thread_count;
}
void
thread_pool_start(thread_pool_t *tp)
{
    ASSERT(tp, "start thread from empty thread pool");
	int i = 0;
	for (; i < tp->thread_count; ++i)
		start_thread(tp->threads + i);
}

void 
thread_pool_stop(thread_pool_t *tp)
{
    ASSERT(tp, "stop thread from empty thread pool");
	int i = 0;
	for (; i < tp->thread_count; ++i)
		stop_thread(tp->threads + i);
}

void                
thread_pool_force_stop(thread_pool_t *tp)
{
    ASSERT(tp, "force stop empty thread pool");
    int i = 0;
	for (; i < tp->thread_count; ++i)
		force_stop_thread(tp->threads + i);
}


static int 
get_next_thread_to_run(thread_pool_t *tp)
{
	pthread_mutex_lock(&tp->rb_lock);
	uint32_t rb = tp->rb;
	tp->rb = (rb + 1) % tp->thread_count;
	pthread_mutex_unlock(&tp->rb_lock);

    return rb;
}

int
thread_pool_add_task_to_run(thread_pool_t *tp, task_func_t func, void *param)
{
    ASSERT (tp, "add task to empty thread pool");
    ASSERT (func, "add empty task to run");
    return add_task_to_thread(tp->threads + get_next_thread_to_run(tp), func, param);
}

void thread_pool_set_thread_data(thread_pool_t *pool, 
                                 int thread_index,
                                 void *user_data)
{
    ASSERT(thread_index >= 0 && thread_index < pool->thread_count, "thread index out of bound\n");
    ASSERT(pool->threads[thread_index].state == IDLE, "only can set data to thread when it's idle");

    pool->threads[thread_index].thread_local_data = user_data;
}

void *thread_pool_get_thread_data(thread_pool_t *pool, int thread_index)
{
    ASSERT(thread_index > 0 && thread_index < pool->thread_count, "thread index out of bound\n");
    return pool->threads[thread_index].thread_local_data;
}


static void
init_thread(thread_pool_t *tp, thread_t *t)
{
    ASSERT(tp && t, "init empty thread pool");
	pthread_mutex_init(&t->task_list_lock, NULL);
    pthread_cond_init(&t->task_list_aval_cond, NULL);
	t->manager = tp;
	t->state = IDLE;
	RING_INIT(&t->tasks, task, link);
	t->mem_pool = mem_pool_create(sizeof(task_t), TASK_POOL_SIZE, false);
}

static void 
start_thread(thread_t *t)
{
    ASSERT(t, "start empty thread");
	t->state = RUNNING;
	pthread_create(&t->tid, NULL, thread_func, (void *)t);
}

static void 
stop_thread(thread_t *t)
{
    ASSERT(t, "stop empty thread");
	t->state = DONE;

    /*
     * signal the condition, in case the thread is blocked on the
     * condition wait. using the lock is to make sure the thread can
     * get the signal if it really blocked on the condition wait
     * */
	pthread_mutex_lock(&t->task_list_lock);
    pthread_cond_signal(&t->task_list_aval_cond);
	pthread_mutex_unlock(&t->task_list_lock);

    //pthread_cancel(t->tid);FIXME by liuxy
    //pthread_detach(t->tid); //liuxy
	pthread_join(t->tid, NULL);
    pthread_cond_destroy(&t->task_list_aval_cond);

	pthread_mutex_destroy(&t->task_list_lock);
    mem_pool_delete(t->mem_pool);
}


static void force_stop_thread(thread_t *t)
{
    pthread_cancel(t->tid);
    pthread_cond_destroy(&t->task_list_aval_cond);

	pthread_mutex_destroy(&t->task_list_lock);
    mem_pool_delete(t->mem_pool);
}


static void * 
thread_func(void *param)
{
	thread_t *t = (thread_t *)param;
	while (1)
	{
		if (t->state == DONE)
			return NULL;

		task_t *task = NULL;
		pthread_mutex_lock(&t->task_list_lock);
		while (RING_EMPTY(&t->tasks, task, link))
		{
			if (t->state == DONE)
				return NULL;
			pthread_cond_wait(&t->task_list_aval_cond, &t->task_list_lock);
		}

		task = RING_FIRST(&t->tasks);
		RING_REMOVE(task, link);
		pthread_mutex_unlock(&t->task_list_lock);
		if (task)
		{
			(*task->func)(&(task->param));
			mem_pool_free(t->mem_pool, (void *)task);
		}
	}

	return NULL;

}

static int
add_task_to_thread(thread_t *t, task_func_t func, void *task_param)
{
	task_t *task = (task_t *)mem_pool_alloc(t->mem_pool);
    if (!task)
        return 1;
	task->func = func;
	task->param.real_time_param = task_param;
	task->param.thread_local_data = t->thread_local_data;
	RING_ELEM_INIT(task, link);

	pthread_mutex_lock(&t->task_list_lock);
    if (RING_EMPTY(&t->tasks, task, link))
        pthread_cond_signal(&t->task_list_aval_cond);
	RING_INSERT_TAIL(&t->tasks, task, task, link);
	pthread_mutex_unlock(&t->task_list_lock);
    return 0;
}

bool
thread_pool_is_idle(thread_pool_t *tp)
{
    int i = 0;
    for (; i < tp->thread_count; i++)
    {
        thread_t *t = tp->threads + i;
        ASSERT(t, "a thread in thread pool is null\n");
        pthread_mutex_lock(&(t->task_list_lock));
        if (!RING_EMPTY(&(t->tasks), task, link))
        {
            pthread_mutex_unlock(&(t->task_list_lock));
            return false;
        }
        pthread_mutex_unlock(&(t->task_list_lock));
    }
    return true;
}
