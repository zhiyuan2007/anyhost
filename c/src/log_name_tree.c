#include <pthread.h>
#include <assert.h>
#include <stdint.h>
#include "log_name_tree.h"


static inline 
int name_compare(const void *value1, const void *value2)
{
    assert(value1 && value2);
    const char *name1 = ((node_value_t *)value1)->fqdn;
    const char *name2 = ((node_value_t *)value2)->fqdn;

    return strcmp(name1, name2);
}

inline int elem_compare(void *e1, void *e2)
{
    assert(e1 && e2);
    node_value_t *t1 = (node_value_t*)e1;
    node_value_t *t2 = (node_value_t*)e2;
    return t1->count - t2->count;
}

inline int elem_insert(void *e1, int idx)
{
    assert(e1);
    node_value_t *nv = (node_value_t *)e1;
    nv->heap_index = idx;
}

inline int elem_traverse(void *e1)
{
    assert(e1);
    node_value_t *nv = (node_value_t *)e1;
    printf("idx %d, cnt %u domain %s\n", nv->heap_index, nv->count, nv->fqdn);
}


name_tree_t *
name_tree_create()
{
    name_tree_t *tree = calloc(1, sizeof(name_tree_t));
    if (!tree)
        return NULL;

    tree->_tree = rbtree_create(name_compare);
    if (!tree->_tree)
        goto MEM_ALLOC_FAILED;

    tree->heap = heap_init(1000, elem_compare);
    if (!tree->heap)
        goto MEM_ALLOC_FAILED;

    tree->node_pool = mem_pool_create(sizeof(rbnode_t), 1024, true);
    if (!tree->node_pool)
        goto MEM_ALLOC_FAILED;

    tree->name_pool = mem_pool_create(sizeof(node_value_t), 1024, true);
    if (!tree->name_pool)
        goto MEM_ALLOC_FAILED;

	tree->lru = lru_list_create();
	if (!tree->lru)
		goto MEM_ALLOC_FAILED;

    tree->count = 0;

    return tree;

MEM_ALLOC_FAILED :
    if (tree->name_pool) mem_pool_delete(tree->name_pool);
    if (tree->node_pool) mem_pool_delete(tree->node_pool);
    if (tree->_tree) free(tree->_tree);
    if (tree->heap) heap_destory(tree->heap);
	if (tree->lru) lru_list_destroy(tree->lru);
    free(tree);
}

uint64_t name_tree_get_size(name_tree_t *tree) 
{
	uint64_t initsize = sizeof(name_tree_t);
	uint64_t heapsize = sizeof(heap_t) + MAX_HEAP_NODE_NUM * sizeof(void *);
	uint64_t mempoolsize = mem_pool_struct_size() * 2;
	uint64_t nodesize = tree->count * (sizeof(rbnode_t) + sizeof(node_value_t));
	uint64_t lrusize = tree->lru->count * sizeof(lru_node_t);
	return initsize + heapsize + mempoolsize + nodesize + lrusize;
}

void
name_tree_destroy(name_tree_t *tree)
{
    mem_pool_delete(tree->node_pool);
    mem_pool_delete(tree->name_pool);
    heap_destory(tree->heap);
	lru_list_destroy(tree->lru);
    free(tree->_tree);
    free(tree);
}

static inline bool
name_end_with_dot(const char *name)
{
    assert(name && strlen(name));
    return name[strlen(name) - 1] == '.';
}

heap_t *name_tree_copy_heap(name_tree_t *tree)
{
    heap_t *hp = heap_copy(tree->heap);
    return hp;
}

int
name_tree_insert(name_tree_t *tree, const char *name)
{
    if (!name || (strlen(name) >= MAX_NAME_LEN))
        return -1;

    node_value_t *nv = name_tree_exactly_find(tree, name);
    if (nv == NULL)
    {
       nv = (node_value_t *)mem_pool_alloc(tree->name_pool);
       if (!nv)
       {
           return -1;
       }

       rbnode_t *node = (rbnode_t *)mem_pool_alloc(tree->node_pool);
       if (!node) {
           mem_pool_free(tree->name_pool, (void *)nv);
           return -1;
       }

       strcpy(nv->fqdn, name);
       if (!name_end_with_dot(name))
           strcat(nv->fqdn, ".");
       nv->count = 0;
       nv->heap_index = -1;
       node->value = (void *)nv;

       rbnode_t *insert_node = rbtree_insert(tree->_tree, node);
       if (!insert_node) {
           mem_pool_free(tree->node_pool, (void *)node);
           mem_pool_free(tree->name_pool, (void *)nv);
           return -1;
       }
	   lru_node_t *lru_node = lru_list_insert(tree->lru, nv);
	   nv->ptr = lru_node;
       tree->count++;
    }else {
		lru_list_move_to_first(tree->lru, nv->ptr);
	}
    nv->count++;
    if (nv->heap_index != -1)
    {
        heap_adjust_siftdown(tree->heap, nv->heap_index, elem_insert);
    }
    else
    {
        void *new_elem =(void *)nv;
        if (heap_reach_roof(tree->heap, 1000))
        {
            if (heap_min_less_than(tree->heap, new_elem))
            {
               heap_replace_least(tree->heap, new_elem, elem_insert);
            }
        }
        else
        {
            int index = heap_insert(tree->heap, new_elem, elem_insert);
        }
    }
    return 0;
}

int
name_tree_delete(name_tree_t *tree,
                    const char *name, del_node del_func)
{
    if (!name || (strlen(name) >= MAX_NAME_LEN))
        return -1;

    node_value_t nv;
    strcpy(nv.fqdn, name);
    if (!name_end_with_dot(name))
        strcat(nv.fqdn, ".");

    rbnode_t *del_node = rbtree_delete(tree->_tree, (void *)&nv);
    if (!del_node)
        return -1;

	if (del_func)
		del_func( tree->lru, ((node_value_t *)del_node->value)->ptr);
    mem_pool_free(tree->name_pool, del_node->value);
    mem_pool_free(tree->node_pool, (void *)del_node);

    tree->count--;
    return 0;
}

node_value_t *
name_tree_exactly_find(name_tree_t *tree,
                       const char *name)
{
    if (!name || (strlen(name) >= MAX_NAME_LEN))
        return NULL;

    node_value_t nv;
    strcpy(nv.fqdn, name);
    if (!name_end_with_dot(name))
        strcat(nv.fqdn, ".");

    rbnode_t *node = rbtree_search(tree->_tree, (void *)&nv);
    if (!node)
    {
        return NULL;
    }
    return node->value;
}

static inline bool
name_is_root(const char *name)
{
    assert(name && strlen(name));
    return (strlen(name) == 1) && (name[0] == '.');
}

static inline const char *
generate_parent_name(const char *name)
{
    assert(name && strlen(name));
    if (name_is_root(name))
        return NULL;

    const char *dot_pos = strchr(name, '.');
    if ((dot_pos - name) == (strlen(name) - 1))
        return dot_pos;
    else
        return dot_pos + 1;
}

node_value_t *
name_tree_closely_find(name_tree_t *tree,
                       const char *name)
{
    if (!name || (strlen(name) >= MAX_NAME_LEN))
        return NULL;

    node_value_t nv;
    strcpy(nv.fqdn, name);
    if (!name_end_with_dot(name))
        strcat(nv.fqdn, ".");

    const char *name_pos = nv.fqdn;
    void *find_name = NULL;

    do {
        if (find_name = name_tree_exactly_find(tree, name_pos))
            return find_name;
    } while (name_pos = generate_parent_name(name_pos));

    return NULL;
}
