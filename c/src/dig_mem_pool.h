#ifndef MEM_POOL_H
#define MEM_POOL_H

#include <stdint.h>
#include <stdbool.h>

/*
 *  memory pool for alloc static sized chuck of memory
 *  it's thread safe
 *  it supports fixed size memory or grow the memory as needed
 * */
struct mem_pool;
typedef struct mem_pool mem_pool_t;

mem_pool_t *	mem_pool_create(uint32_t elem_size,
                                uint32_t elem_count, 
                                bool needs_auto_grow);
void 	    	mem_pool_delete(mem_pool_t *mp);
void*		    mem_pool_alloc(mem_pool_t *mp);
void		    mem_pool_free(mem_pool_t *mp,void *mem);
uint32_t mem_pool_struct_size();
	
#endif
