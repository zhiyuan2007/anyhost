#ifndef HEADER_LOG_VIEW_TREE__H
#define HEADER_LOG_VIEW_TREE__H
#include "dig_rb_tree.h"
#include "log_view_stats.h"
#include "zip_lru_list.h"
#define STATS_NODE_MAX_SIZE 128
typedef struct view_tree {
	rbtree_t *rbtree;
	lru_list_t *lrulist;
	int count;
}view_tree_t;

struct view_tree_node {
    char name[STATS_NODE_MAX_SIZE];
    view_stats_t *vs;
	void *ptr;
};
view_tree_t *view_tree_create(int (cmp)(const void *, const void *));
uint64_t view_tree_get_size(view_tree_t *tree);
typedef struct view_tree_node view_tree_node_t;
int view_tree_insert(view_tree_t *tree, const char *name, view_stats_t *vs);
void *view_tree_find(view_tree_t *tree, const char *name);
void view_tree_delete(view_tree_t *tree, const char *name);
void view_tree_destory(view_tree_t *tree);
#endif
