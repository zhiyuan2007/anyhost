#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>
#include "log_heap.h"

heap_t *heap_init(int ms, compare_func cmp)
{
    if (ms <= 0){
        return NULL;
    }
    if (ms > MAX_HEAP_NODE_NUM)
    {
        printf("heap size too big\n");
        return NULL;
    }
    
    heap_t *hbt = malloc(sizeof(heap_t));
    if (hbt == NULL)
    {
        printf("out of memory\n");
        return NULL;
    }

    hbt->maxSize = ms;
    hbt->len = 0;
    hbt->cmp = cmp;

    return hbt;
}

void heap_destory(heap_t *hbt)
{
    assert(hbt);
    free(hbt);
}

void *heap_value(heap_t *hbt, int i)
{
    return hbt->heap[i];
}

bool heap_empty(heap_t *hbt)
{
    return 0 == hbt->len;
}

bool heap_reach_roof(heap_t *hbt, int roof)
{
    return  hbt->len >= roof;
}

void heap_traverse(heap_t *hbt, traverse_func func)
{
    int i = 0;
    for (; i < hbt->len; i++)
        func(hbt->heap[i]);
}

bool heap_min_less_than(heap_t *hbt, void *elem)
{

    return hbt->cmp(hbt->heap[0], elem) < 0 ;
}

int heap_insert(heap_t *hbt, void *x, insert_func func)
{
    int i;
    hbt->heap[hbt->len] = x;
    if (func != NULL)
        func(x, hbt->len);
    hbt->len++;

    i = hbt->len - 1;

    while (0 != i){
        int j = (i - 1) / 2;
        if (hbt->cmp(hbt->heap[j], hbt->heap[i]) <= 0){        
            break;
        }
        hbt->heap[i] = hbt->heap[j];
        if (func != NULL)
            func(hbt->heap[j], i);
        i = j;
    }
    hbt->heap[i] = x;
    if (func != NULL)
        func(x, i);

    return i;
}

void *heap_delete(heap_t *hbt)
{
    void *temp, *x;
    int i, j;
    
    if (0 == hbt->len){
        printf("out of memory\n");
        exit(1);
    }
    
    temp = hbt->heap[0];
    hbt->len--;

    if (0 == hbt->len){
        return temp;
    }

    x = hbt->heap[hbt->len];
    i = 0;
    j = 2 * i + 1;
    while (j <= hbt->len - 1){
        if ((j < hbt->len - 1) && (hbt->cmp(hbt->heap[j], hbt->heap[j+1]) > 0)){
            j++;
        }
        if (hbt->cmp(x, hbt->heap[j]) <= 0){
            break;
        }
        hbt->heap[i] = hbt->heap[j];
        i = j;
        j = 2 * i + 1;
    }
    hbt->heap[i] = x;
    
    return temp;
}
heap_t *heap_copy(heap_t *hbt)
{
    heap_t *copy_heap = malloc(sizeof(heap_t ));
    if (copy_heap== NULL)
    {
        printf("out of memory\n");
        return NULL;
    }

    int i;
    for (i = 0; i < hbt->len; i++)
    {
        copy_heap->heap[i] = hbt->heap[i];
    }
    copy_heap->len = hbt->len;
    copy_heap->maxSize = hbt->maxSize;
    copy_heap->cmp = hbt->cmp;
    return copy_heap;
}

void heap_sort(heap_t *hbt)
{
    int len = hbt->len;
    for (len = hbt->len; len>0; len--)
    {
       void *elem;
       elem = hbt->heap[len-1];
       hbt->heap[len-1] = hbt->heap[0];
       hbt->heap[0] = elem;
       if (len == 2) 
           break;

       int i = 0;
       int j = 2 * i + 1;
       while (j <= len - 2){
           if ((j < len - 2) && (hbt->cmp(hbt->heap[j], hbt->heap[j+1])>0)){
               j++;
           }
           if (hbt->cmp(elem , hbt->heap[j]) <= 0){
               break;
           }
           hbt->heap[i] = hbt->heap[j];
           i = j;
           j = 2 * i + 1;
       }
       hbt->heap[i] = elem;
    }
}

void heap_adjust_siftdown(heap_t *hbt, int index, insert_func func)
{
    void *elem = heap_value(hbt, index);
    int i = index;
    int j = 2 * i + 1;
    while (j <= hbt->len - 1){
        if ((j < hbt->len - 1) && (hbt->cmp(hbt->heap[j], hbt->heap[j+1]) >0)){
            j++;
        }
        if (hbt->cmp(elem,  hbt->heap[j]) <= 0){
            break;
        }
        hbt->heap[i] = hbt->heap[j];
        if (func != NULL)
            func(hbt->heap[j], i);
        i = j;
        j = 2 * i + 1;
    }
    if (i != index)
    {
        hbt->heap[i] = elem;
        if (func != NULL)
            func(elem, i);
    }
}

void *heap_replace_least(heap_t *hbt, void *new, insert_func func )
{
    void *temp = hbt->heap[0];
    if (hbt->cmp(new, temp) <= 0)
        return new;

    int i = 0;
    int j = 2 * i + 1;
    while (j <= hbt->len - 1){
        if ((j < hbt->len - 1) && (hbt->cmp(hbt->heap[j], hbt->heap[j+1]) >0)){
            j++;
        }
        if (hbt->cmp(new,  hbt->heap[j]) <= 0){
            break;
        }
        hbt->heap[i] = hbt->heap[j];
        if (func != NULL)
            func(hbt->heap[j], i);
        i = j;
        j = 2 * i + 1;
    }
    hbt->heap[i] = new;
    if (func != NULL)
        func(new, i);
    
    return temp;
}

