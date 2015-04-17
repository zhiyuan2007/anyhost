/***
 *DynaLnkQueue.c
 *
 *
 *
 ****/

#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <assert.h>
#include "dynamic_queue.h"

#define DEFAULE_QUEUE_SIZE 100

link_queue_t *
dqueue_create()
{
    return dqueue_create_with_size(DEFAULE_QUEUE_SIZE);
}

link_queue_t *
dqueue_create_with_size(int queue_size)
{
    link_queue_t *q = (link_queue_t *)malloc(sizeof(link_queue_t));
    assert(q != NULL);

    q->data = (void **)malloc(queue_size * sizeof(void *));
    assert(q->data != NULL);

    q->front = q->rear = 0;
    q->queue_size = queue_size;

    return q;
}

bool dqueue_empty(link_queue_t *q)
{
    assert(q != NULL);

    if(q->front == q->rear)
        return true;
    else
        return false;
}

bool dqueue_full(link_queue_t *q)
{
    assert(q != NULL);

    if (q->front == (q->rear + 1) % q->queue_size)
        return true;
    else
        return false;
}

int  dqueue_length(link_queue_t *q)
{
    assert(q!= NULL);

    return (q->rear - q->front + q->queue_size) % q->queue_size;
}

//insert node in queue's rear
bool dqueue_enqueue(link_queue_t *q, void *e)
{
    assert(q != NULL);

    if (dqueue_full(q))
        return false;

    q->data[q->rear] = e;
    q->rear = (q->rear + 1) % q->queue_size;
    return true;
}

void *dqueue_dequeue(link_queue_t *q)
{
    assert(q != NULL);

    if(dqueue_empty(q))
    {
        return NULL;
    }

    void *e = q->data[q->front];
    q->front= (q->front + 1) % q->queue_size;

    return e;
}

void dqueue_destroy(link_queue_t *q)
{
    assert(q != NULL);

    void *e = NULL;

    while ((e = dqueue_dequeue(q)) != NULL)
    {
        free(e);
        e = NULL;
    }

    free(q);
    q = NULL;
}
