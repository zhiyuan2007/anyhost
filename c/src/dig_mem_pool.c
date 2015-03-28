#include <stdlib.h>
#include <pthread.h>

#include "dig_mem_pool.h"
#include "dig_ring.h"
#include "log_utils.h"

/*
 * mem pool manage several raw memory block, each block will contain fix size memory element which
 * will be allocate for end user.
*/

#define DEFAULT_ELEM_COUNT 20 

#define ALIGN_PAD(size) \
    (((size) % sizeof(void *) == 0) ? 0 : (((size) | ~-(int)sizeof(void *)) + 1 - (size)))

typedef struct mem_elem
{
    struct mem_elem *next;
}mem_elem_t;

typedef struct mem_chunk
{
    struct mem_chunk *next;
}mem_chunk_t;


struct mem_pool
{
	uint32_t		    elem_size;
	uint32_t		    elem_count;

	mem_elem_t          *free_list;
    mem_chunk_t         *raw_mem_list;
	pthread_mutex_t 	list_lock;

    bool                needs_auto_grow;
};

uint32_t mem_pool_struct_size() {
	return sizeof(mem_pool_t);
}

static void grow_new_mem(mem_pool_t *mp, uint32_t new_elem_count);
static void fill_free_list(mem_pool_t *mp, 
                           uint8_t *raw_mem,
                           uint32_t elem_count);

mem_pool_t * 
mem_pool_create(uint32_t elem_size, 
                unsigned int elem_count, 
                bool needs_auto_grow)
{
	mem_pool_t *mp;
    DIG_CALLOC(mp, 1, sizeof(mem_pool_t));

	int size = elem_size > sizeof(mem_elem_t) ? elem_size : sizeof(mem_elem_t);
    mp->elem_size = size + ALIGN_PAD(size);
	mp->elem_count = 0;

    mp->free_list = NULL;
    mp->raw_mem_list = NULL;
    mp->needs_auto_grow = needs_auto_grow;

    grow_new_mem(mp, (elem_count == 0 ? DEFAULT_ELEM_COUNT : elem_count));
	pthread_mutex_init(&mp->list_lock,NULL);
	return mp;
}

static void 
grow_new_mem(mem_pool_t *mp, uint32_t new_elem_count)
{
    mem_chunk_t *new_mem = (mem_chunk_t *)malloc(sizeof(mem_chunk_t *) + new_elem_count * mp->elem_size);
    if (new_mem == NULL) return;

    new_mem->next = mp->raw_mem_list;
    mp->raw_mem_list = new_mem;
    fill_free_list(mp, (uint8_t *)(new_mem + 1), new_elem_count);

    mp->elem_count += new_elem_count;
}

static void 
fill_free_list(mem_pool_t *mp,  uint8_t *raw_mem, uint32_t elem_count)
{
    int i;
    for (i = 0;i < elem_count; ++i)
    {
        mem_elem_t *e = (mem_elem_t *)((uint8_t *)raw_mem + i * mp->elem_size);
        e->next = mp->free_list;
        mp->free_list = e;
    }
}

void 
mem_pool_delete(mem_pool_t *mp)
{
    ASSERT(mp,"delete empty mem pool");
	pthread_mutex_destroy(&mp->list_lock);
    mem_chunk_t *head = mp->raw_mem_list;
    while (head)
    {
        mem_chunk_t *tmp = head->next;
        free(head);
        head = tmp; 
    }
	free(mp);
}

void *
mem_pool_alloc(mem_pool_t *mp)
{
    ASSERT(mp,"alloc memory from empy mem pool");

	pthread_mutex_lock(&mp->list_lock);
    if (mp->free_list == NULL && mp->needs_auto_grow)
        grow_new_mem(mp, mp->elem_count * 2);

    mem_elem_t *e = NULL;
    if (mp->free_list != NULL)
	{
        e = mp->free_list;
        mp->free_list = e->next;
	}

	pthread_mutex_unlock(&mp->list_lock);

	return (void *)e;
}


void 
mem_pool_free(mem_pool_t *mp, void *mem)
{
    ASSERT(mp,"free memory to empty mem pool");
    ASSERT(mem, "free empty memory to mem pool");

	pthread_mutex_lock(&mp->list_lock);
    mem_elem_t *e = (mem_elem_t *)mem;
    e->next = mp->free_list;
    mp->free_list = e;
	pthread_mutex_unlock(&mp->list_lock);
}
