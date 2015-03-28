#ifndef _H_LOG_NAME_TREE_
#define _H_LOG_NAME_TREE_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "log_heap.h"
#include "dig_mem_pool.h"
#include "dig_rb_tree.h"
#include "zip_lru_list.h"

#define MAX_NAME_LEN    32

typedef struct name_tree name_tree_t;
typedef struct node_value node_value_t;
typedef struct node_list node_list_t;
struct node_value{
    char fqdn[MAX_NAME_LEN];
    uint32_t count;
    int heap_index;
	void *ptr;
};

struct name_tree {
    rbtree_t    *_tree;
    heap_t      *heap;
    mem_pool_t  *node_pool;
    mem_pool_t  *name_pool;
    uint32_t    count;
    pthread_rwlock_t rwlock;
	lru_list_t *lru;
};

typedef (*del_node)(void *manager, void *data);

name_tree_t *   name_tree_create();
uint64_t name_tree_get_size(name_tree_t *tree); 
void            name_tree_destroy(name_tree_t *tree);
int name_tree_insert(name_tree_t *tree,
                        const char *name);
int             name_tree_delete(name_tree_t *tree,
                        const char *name, del_node del_func);
node_value_t *name_tree_exactly_find(name_tree_t *tree,
                        const char *name);
node_value_t *name_tree_closely_find(name_tree_t *tree,
                        const char *name);
int name_tree_topn(name_tree_t *tree, int topn, char *buff);

#endif  /* _H_LOG_NAME_TREE_ */
