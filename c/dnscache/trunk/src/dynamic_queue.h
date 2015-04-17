#ifndef DYNALNKQUEUE_H
#define DYNALNKQUEUE_H

#include <stdbool.h>
#include "dig_atomic.h"

typedef struct dqueue dqueue_t;

dqueue_t *
dqueue_create();

dqueue_t *
dqueue_create_with_size(int queue_size);

void
dqueue_destroy(dqueue_t *q);

int
dqueue_lock(dqueue_t *q);

int
dqueue_trylock(dqueue_t *q);

int
dqueue_unlock(dqueue_t *q);

bool
dqueue_is_empty(dqueue_t *q);

bool
dqueue_is_full(dqueue_t *q);

int
dqueue_length(dqueue_t *q);

bool
dqueue_enqueue(dqueue_t *q, void *e);

void
*dqueue_dequeue(dqueue_t *q);

#endif /* dynalnkqueue_h */
