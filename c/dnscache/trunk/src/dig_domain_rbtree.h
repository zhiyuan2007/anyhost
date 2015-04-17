#ifndef _H_DIG_NAME_TREE_
#define _H_DIG_NAME_TREE_

#include "dig_rb_tree.h"
#include "dig_wire_name.h"

typedef struct name_tree name_tree_t;

typedef void (*value_del_func)(void *value);

name_tree_t *   name_tree_create(int max_node_count, value_del_func del_func);
void            name_tree_destroy(name_tree_t *ntree);
void            name_tree_clear(name_tree_t *ntree);
rbnode_t *      name_tree_insert(name_tree_t *ntree, const wire_name_t *name);
bool            name_tree_delete(name_tree_t *ntree, const wire_name_t *name);
rbnode_t *      name_tree_find(name_tree_t *ntree, const wire_name_t *name);

#endif  /* _H_DIG_NAME_TREE_ */
