#ifndef __HEADER_123_LOG_HEAP__H
#define __HEADER_123_LOG_HEAP__H
#define MAX_HEAP_NODE_NUM 10000
typedef int (*compare_func)(void *e1, void *e2);
typedef int (*insert_func)(void *e1, int idx);
typedef int (*traverse_func)(void *e1);

struct heap{
    void *heap[MAX_HEAP_NODE_NUM];
    int len; 
    int maxSize;
    compare_func cmp;
};


typedef struct heap heap_t;
heap_t *heap_init(int ms, compare_func cmp);
void heap_destory(heap_t *hbt);
void *heap_value(heap_t *hbt, int i);
bool heap_empty(heap_t *hbt);
bool heap_reach_roof(heap_t *hbt, int roof);
void heap_traverse(heap_t *hbt, traverse_func func);
bool heap_min_less_than(heap_t *hbt, void *elem);
int heap_insert(heap_t *hbt, void *x, insert_func func);
void *heap_delete(heap_t *hbt);
void heap_sort(heap_t *hbt);
void heap_adjust_siftdown(heap_t *hbt, int index, insert_func func);
void *heap_replace_least(heap_t *hbt, void *new, insert_func func);
heap_t *heap_copy(heap_t *hbt);
#endif
