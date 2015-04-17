#include <stdlib.h>
#include <memory.h>
#include <assert.h>
#include <pthread.h>
#include "dynamic_queue.h"

#define DEFAULE_QUEUE_SIZE 100
#define LOCK_QUEUE(q) do{pthread_mutex_lock(&(q)->mut);}while(0)
#define UNLOCK_QUEUE(q) do{pthread_mutex_unlock(&(q)->mut);}while(0)


static inline bool  _dqueue_is_empty(dqueue_t *q);
static inline bool  _dqueue_is_full(dqueue_t *q);
static inline int _dqueue_length(const dqueue_t *q);
struct dqueue
{
    void **data;       // elem data
    int front;
    int rear;
    int queue_size;        //pre malloc size
    pthread_mutex_t mut;
};

dqueue_t *
dqueue_create()
{
    return dqueue_create_with_size(DEFAULE_QUEUE_SIZE);
}

dqueue_t *
dqueue_create_with_size(int queue_size)
{
    dqueue_t *q = (dqueue_t *)malloc(sizeof(dqueue_t));
    assert(q != NULL);
    q->data = (void **)malloc(queue_size * sizeof(void *));
    assert(q->data != NULL);
    q->front = q->rear = 0;
    q->queue_size = queue_size;
    pthread_mutex_init(&q->mut, NULL);

    return q;
}


bool
dqueue_is_empty(dqueue_t *q)
{
    LOCK_QUEUE(q);
    bool ret = _dqueue_is_empty(q);
    UNLOCK_QUEUE(q);
    return ret;
}
    
static inline bool
_dqueue_is_empty(dqueue_t *q)
{
    return q->front == q->rear;
}



bool
dqueue_is_full(dqueue_t *q)
{
    LOCK_QUEUE(q);
    bool ret = _dqueue_is_full(q);
    UNLOCK_QUEUE(q);
    return ret;
}

static inline bool
_dqueue_is_full(dqueue_t *q)
{
    return q->front == (q->rear + 1) % q->queue_size;
}

int
dqueue_length(dqueue_t *q)
{
    LOCK_QUEUE(q);
    int len = _dqueue_length(q);
    UNLOCK_QUEUE(q);
    return len;
}

static inline int
_dqueue_length(const dqueue_t *q)
{
    return (q->rear - q->front + q->queue_size) % q->queue_size;
}

bool
dqueue_enqueue(dqueue_t *q, void *e)
{
    LOCK_QUEUE(q);
    if (_dqueue_is_full(q))
    {
        UNLOCK_QUEUE(q);
        return false;
    }

    q->data[q->rear] = e;
    q->rear = (q->rear + 1) % q->queue_size;
    UNLOCK_QUEUE(q);
    return true;
}

void *
dqueue_dequeue(dqueue_t *q)
{
    LOCK_QUEUE(q);
    if(_dqueue_is_empty(q))
    {
        UNLOCK_QUEUE(q);
        return NULL;
    }

    void *e = q->data[q->front];
    q->front= (q->front + 1) % q->queue_size;
    UNLOCK_QUEUE(q);
    return e;
}

void
dqueue_destroy(dqueue_t *q)
{
    pthread_mutex_destroy(&(q)->mut);
    free(q);
    q = NULL;
}
