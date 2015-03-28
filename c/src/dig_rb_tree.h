#ifndef _H_DIG_RBTREE_H_
#define	_H_DIG_RBTREE_H_

#include <stdarg.h>
#include <stdint.h>
#include <string.h>

/*
 * This structure must be the first member of the data structure in
 * the rbtree.  This allows easy casting between an rbnode_t and the
 * user data (poor man's inheritance).
 */

typedef struct rbnode_t rbnode_t;
struct rbnode_t {
	rbnode_t    *parent;
	rbnode_t    *left;
	rbnode_t    *right;
    void        *value;
	uint8_t	    color;
};

#define	RBTREE_NULL &rbtree_null_node
extern	rbnode_t	rbtree_null_node;

typedef struct rbtree_t rbtree_t;
struct rbtree_t {
	/* The root of the red-black tree */
	rbnode_t    *root;

	/* The number of the nodes in the tree */
	size_t       count;

	/* Current node for walks... */
	rbnode_t    *_node;

	/* Key compare function. <0,0,>0 like strcmp. Return 0 on two NULL ptrs. */
	int (*cmp) (const void *, const void *);
};

typedef void (destroy_func)(void *ptr);
/* rbtree.c */
rbtree_t *rbtree_create(int (*cmpf)(const void *, const void *));
rbnode_t *rbtree_insert(rbtree_t *rbtree, rbnode_t *data);
/* returns node that is now unlinked from the tree. User to delete it. 
 * returns 0 if node not present */
rbnode_t *rbtree_delete(rbtree_t *rbtree, const void *key);
rbnode_t *rbtree_search(rbtree_t *rbtree, const void *key);
/* returns true if exact match in result. Else result points to <= element,
   or NULL if key is smaller than the smallest key. */
int rbtree_find_less_equal(rbtree_t *rbtree, const void *key, rbnode_t **result);
rbnode_t *rbtree_first(rbtree_t *rbtree);
rbnode_t *rbtree_last(rbtree_t *rbtree);
rbnode_t *rbtree_next(rbnode_t *rbtree);
rbnode_t *rbtree_previous(rbnode_t *rbtree);

void rbtree_destory(rbtree_t *tree, destroy_func func);

#define	RBTREE_WALK(rbtree, v, d) \
	for((rbtree)->_node = rbtree_first(rbtree);\
		(rbtree)->_node != RBTREE_NULL && ((v) = (rbtree->_node->value)) && \
		((d) = (rbtree)->_node); (rbtree)->_node = rbtree_next((rbtree)->_node))

/* call with node=variable of struct* with rbnode_t as first element.
   with type is the type of a pointer to that struct. */
#define RBTREE_FOR(node, type, rbtree) \
	for(node=(type)rbtree_first(rbtree); \
		(rbnode_t*)node != RBTREE_NULL; \
		node = (type)rbtree_next((rbnode_t*)node))

#endif /* _H_DIG_RBTREE_H_ */
