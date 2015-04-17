/*
 * @file memory_pool.h
 * @brief  memory pool management functions
 * @author Liu Guirong
 * @date 2011-07-11
 */
#ifndef MEMORY_POOL_H
#define MEMORY_POOL_H

typedef struct mem_pool mem_pool_t;
/*!
 * @param n_unit_size [I] memory pool unit size
 * @param n_init_size [I] initial allocated unit number
 * @param n_grow_size [I] grow number
 * @return if initialise successful return ZIP_SUCCESS or return ZIP_FAILURE
 */
extern mem_pool_t *
mem_pool_create(uint32_t unit_size, uint32_t init_size, uint32_t grow_size);

/*!
 * @param mp memory pool pointer
 * @return return memory pool pointer if success ,or return NULL
 * @note  allocated a new node
 */
extern void*
mem_pool_alloc(mem_pool_t *mp);

/*!
 * @param mp [I] memory pool pointer
 * @param p [I] the node that need free
 * @return success return ZIP_SUCCESS ,or return ZIP_FAILURE
 */
extern int
mem_pool_free(mem_pool_t *mp, void* p);

/*
 * @param mp [I]  memory pool pointer
 * @note  free all memory
 */
void
mem_pool_delete(mem_pool_t *mp);

#endif
