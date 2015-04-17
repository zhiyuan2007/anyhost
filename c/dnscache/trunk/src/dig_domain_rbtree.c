#include "dig_domain_rbtree.h"

struct name_tree
{
    rbtree_t        *tree;
    mem_pool_t      *pool;
    value_del_func  func_delete;
};

//typedef void (*value_del_func)(void *value);

static  int     tree_key_cmp(const void *key1, const void *key2);

name_tree_t *
name_tree_create(int max_node_count, value_del_func del_func)
{
    name_tree_t *ntree = malloc(sizeof(name_tree_t));
    if (!ntree)
        goto TREE_FAILED;

    mem_pool_t *mpool = mem_pool_create(sizeof(rbnode_t), max_node_count, true);
    if (!mpool)
        goto MEMPOOL_FAILED;

    rbtree_t *tree = rbtree_create(mpool, tree_key_cmp);
    if (!tree)
        goto RBTREE_FAILED;

    ntree->tree = tree;
    ntree->pool = mpool;
    ntree->func_delete = del_func;

    return ntree;

RBTREE_FAILED :
    mem_pool_delete(mpool);
MEMPOOL_FAILED :
    free(ntree);
TREE_FAILED :
    return NULL;
}

void
name_tree_destroy(name_tree_t *ntree)
{
    name_tree_clear(ntree);
    free(ntree->tree);
    mem_pool_delete(ntree->pool);
    free(ntree);
}

void
name_tree_clear(name_tree_t *ntree)
{
    void  *key = NULL;
    void  *value = NULL;
    rbnode_t    *node = NULL;
    RBTREE_WALK(ntree->tree, key, value, node)
    {
        ntree->func_delete(value);
        wire_name_delete((wire_name_t *)key);
        node->mkey = NULL;
        node->value = NULL;
    }
}

rbnode_t *
name_tree_insert(name_tree_t *ntree, const wire_name_t *name)
{
    rbnode_t *node = (rbnode_t *)mem_pool_alloc(ntree->pool);
    if (!node)
        return NULL;

    node->mkey = (void *)wire_name_clone(name);

    rbnode_t *insert_node = rbtree_insert(ntree->tree, node);
    if (insert_node != node)
    {
        wire_name_delete((wire_name_t *)node->mkey);
        mem_pool_free(ntree->pool, node);
    }
    else
        insert_node->value = NULL;

    return insert_node;
}

bool
name_tree_delete(name_tree_t *ntree, const wire_name_t *name)
{
    rbnode_t *del_node = rbtree_delete(ntree->tree, (void *)name);
    if (!del_node)
        return false;

    //ASSERT(del_node->value, "the value of delete node must exist\n");
    if (del_node->value)
        ntree->func_delete(del_node->value);
    wire_name_delete((wire_name_t *)del_node->mkey);
    mem_pool_free(ntree->pool, (void *)del_node);

    return true;
}

rbnode_t *
name_tree_find(name_tree_t *ntree, const wire_name_t *name)
{
    return rbtree_search(ntree->tree, (const void *)name);
}


static int
tree_key_cmp(const void *key1, const void *key2)
{
    ASSERT(key1 && key2, "cmp parameters must not be null\n");

    return wire_name_compare((const wire_name_t *)key1, (const wire_name_t *)key2);
}
