#ifndef _H_DIG_IP_TREE_H_
#define _H_DIG_IP_TREE_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

typedef struct radix_tree   radix_tree_t;
typedef struct radix_node   radix_node_t;
typedef enum
{
    SUCCEED = 0,
    NO_MEMORY,
    DUP_KEY,
    FORMAT_ERROR,
    FAILED = 1001
} result_t;

typedef void (*value_func)(void *value);

const char *
radix_tree_get_result(result_t result);

radix_tree_t *
radix_tree_create(value_func value_del_function);

void
radix_tree_delete(radix_tree_t *tree);

void
radix_tree_walk(radix_tree_t *tree, value_func value_function);

result_t
radix_tree_insert_subnet(radix_tree_t *tree, const char *subnet, void* info);

result_t
radix_tree_insert_node(radix_tree_t *tree, uint32_t key,
                                           uint32_t mask, void *value);
result_t
radix_tree_remove_subnet(radix_tree_t *tree, const char* subnet);

result_t
radix_tree_delete_node(radix_tree_t *tree, uint32_t key, uint32_t mask);

void *
radix_tree_find(radix_tree_t *tree, uint32_t key);

void *
radix_tree_find_str(radix_tree_t *tree, char *strip);

uint32_t
radix_tree_get_ip_size(radix_tree_t *tree);

typedef void (* traverse_func)(void *node);
void
radix_tree_print(radix_tree_t *tree, traverse_func func);

#endif /* _H_DIG_IP_TREE_H_ */
