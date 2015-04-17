#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "zip_utils.h"

#include "zip_memory_pool.h"

#define ADATAUNIT 4
#define ALIGNMENT 4     
#define DEFAULT_MEM_INIT_SIZE 1024 
#define DEFAULT_MEM_GROW_SIZE 1024

typedef struct mem_block
{
    struct mem_block*   next; 
    uint32_t            size;
    uint32_t            left;
    uint32_t            first_index;
    char                adata[1];
}mem_block_t;

struct mem_pool
{
    mem_block_t*     block;
    uint32_t         unit_size;
    uint32_t         init_size;
    uint32_t         grow_size;
};

mem_block_t *
block_new(uint32_t number, uint32_t unit_size)
{
    mem_block_t *my_block = malloc(sizeof(mem_block_t) + number * unit_size);
    if (my_block)
    {
        my_block->size = number;
        my_block->left = number;
        my_block->first_index = 0;
        my_block->next = NULL;
    }

    return my_block;
}

static void
init_block(mem_block_t *new_block, uint32_t size, uint32_t unit_size)
{
    uint32_t i = 1;
    for (; i < size; ++i)
    {
        *((uint32_t *)(new_block->adata + unit_size * i + ADATAUNIT)) = i + 1;
    }

    new_block->first_index = 1;
    new_block->left = size - 1;
}

static void
block_delete(mem_block_t *block)
{
    free(block);
}

mem_pool_t *
mem_pool_create(uint32_t unit_size, uint32_t init_size, uint32_t grow_size)
{
    mem_pool_t *mp = malloc(sizeof(mem_pool_t));
    if (!mp)
        return NULL;

    mp->block = NULL;
    
    mp->init_size = (init_size == 0 ? DEFAULT_MEM_INIT_SIZE : init_size);
    mp->grow_size = (grow_size == 0 ? DEFAULT_MEM_GROW_SIZE : grow_size);

    if (unit_size > 4)
       mp->unit_size = (unit_size + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1);
    else
       mp->unit_size = 4;

    return mp;
}

void *
mem_pool_alloc(mem_pool_t *mp)
{
    ASSERT(mp , "no more memory for new node \n");

    if (!mp->block )
    {
        mp->block = block_new(mp->init_size, mp->unit_size);
        init_block(mp->block, mp->init_size, mp->unit_size);

        return (void *)(mp->block->adata + ADATAUNIT);
    }

    mem_block_t *my_block = mp->block;  

    while(my_block && my_block->left == 0)
        my_block = my_block->next;

    if (my_block)
    {
        char *free = (char *)((uint32_t)my_block->adata
                        + (my_block->first_index * mp->unit_size) + ADATAUNIT);

        my_block->first_index = *((uint32_t *)free);
        my_block->left--;

        return (void *)free;
    }
    else 
    {
        my_block = block_new(mp->grow_size, mp->unit_size);
        if (!my_block )
        {
            return NULL;
        }

        my_block->next = mp->block; 
        mp->block = my_block;

        init_block(my_block, mp->grow_size, mp->unit_size);

        return (void *)(my_block->adata + ADATAUNIT);
    }
}

int
mem_pool_free(mem_pool_t *mp, void *free)
{
    ASSERT(mp && free, "mem pool or pointer that freed is NULL");

    uint32_t *pfree_us = (uint32_t *)free;
    mem_block_t* my_block = mp->block;
    mem_block_t* pre_block = NULL;

    while (((uint32_t)my_block->adata > (uint32_t)pfree_us)
        || ((uint32_t)pfree_us >= ((uint32_t)my_block->adata
                                    + my_block->size * mp->unit_size)))
    { 
        pre_block = my_block;
        my_block = my_block->next;
    }

    ASSERT(my_block, "mem pool hasn't this mem");

    my_block->left++;

    *pfree_us = my_block->first_index; 

    uint32_t free_index = ((uint32_t)pfree_us
                      - (uint32_t)my_block->adata - ADATAUNIT) / mp->unit_size;

    my_block->first_index = free_index;

    if (my_block->left == my_block->size)
    {
        if (pre_block)
            pre_block->next = my_block->next;
        else
            mp->block = my_block->next; 
        block_delete(my_block);
    }

    return 0;
}

void
mem_pool_delete(mem_pool_t *mp)
{
    ASSERT(mp, "mem pool delelte parameter is NULL");

    mem_block_t* my_block = mp->block;
    mem_block_t* next_block = NULL;

    while (my_block)
    {
        next_block = my_block->next;
        block_delete(my_block);
        my_block = next_block;
    }
    free(mp);
}
