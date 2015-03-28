#ifndef ZIP_LRU_LIST_HEADER_H
#define ZIP_LRU_LIST_HEADER_H
typedef struct lru_node
{
	struct lru_node *prev;
	struct lru_node *next;
	void *data;
}lru_node_t;

struct lru_list
{
    lru_node_t *header;
    lru_node_t *tail;
    int count;
};


typedef struct lru_list lru_list_t;

lru_list_t * 
lru_list_create();

typedef *(lru_delete_func)(void *data);

void
lru_list_destroy(lru_list_t *lru);

void *
lru_list_remove_tail(lru_list_t *lru);

void
lru_list_delete(lru_list_t *lru, lru_node_t *node);

lru_node_t *
lru_list_search(lru_list_t *lru_list, void *user_data);

lru_node_t *
lru_list_insert(lru_list_t *lru_list, void *user_data);

void
lru_list_move_to_first(lru_list_t *lru_list, struct lru_node *node);

int
lru_list_get_count(lru_list_t *lru_list);

void
lru_list_print(const lru_list_t *lru_list);

#endif
