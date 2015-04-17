/***
 *DynaLnkQueue.h - 动态链式队列的定义
 *  
 ****/

#ifndef DYNALNKQUEUE_H
#define DYNALNKQUEUE_H

#include <stdbool.h>
#include "dig_atomic.h"

typedef struct
{
    void **data;       // elem data
    int front;
    int rear;
    atomic_t atomic;
    int queue_size;        //pre malloc size
} link_queue_t;

/*------------------------------------------------------------
// operate
------------------------------------------------------------*/
link_queue_t *dqueue_create();

link_queue_t *dqueue_create_with_size(int queue_size);

void dqueue_destroy(link_queue_t *q);

bool dqueue_empty(link_queue_t *q);

int  dqueue_length(link_queue_t *q);

bool dqueue_enqueue(link_queue_t *q, void *e);

void *dqueue_dequeue(link_queue_t *q);

int dqueue_lock(link_queue_t *q);

int dqueue_unlock(link_queue_t *q);

#endif /* dynalnkqueue_h */
