#include "dig_radix_tree.h"
#include <arpa/inet.h>
#include <string.h>

struct radix_node 
{
    radix_node_t   *right;
    radix_node_t   *left;
    radix_node_t   *parent;
    void        *value;
};

struct radix_tree 
{
    radix_node_t   *root;
    radix_node_t   *free;
    value_func  value_delete_func;
    uint32_t count;
};

#define ZIP_RADIX_NO_VALUE  (void *)NULL
#define NODE_IS_EMPTY(node) (ZIP_RADIX_NO_VALUE == ((node)->value))

static void *
radix_tree_alloc(radix_tree_t *tree);
static void
ip_node_delete(radix_node_t *node, value_func del_func);
//static radix_node_t *  radix_tree_get_leftest_node(radix_tree_t *tree);
//static radix_node_t *  ip_node_next(radix_node_t *node);
static void
radix_tree_walk_helper(radix_node_t *node, value_func value_function);

const char *
radix_tree_get_result(result_t result)
{       
    switch(result)
    {   
    case SUCCEED:
        return "success";   
    case FAILED:
        return "not exists";
    case NO_MEMORY:
        return "no enough memory";
    case DUP_KEY:
        return "ip already exists";
    case FORMAT_ERROR:
        return "failed ,perhaps format error";
    default:
        return "unkown reason"; 
    }
}


radix_tree_t *
radix_tree_create(value_func del_func)
{
    radix_tree_t *tree = (radix_tree_t *)malloc(sizeof(radix_tree_t));
    if (!tree)
        return NULL;

    tree->free = NULL;
    tree->value_delete_func = del_func;

    tree->root = radix_tree_alloc(tree);
    if (!(tree->root))
        return NULL;

    tree->root->right = NULL;
    tree->root->left = NULL;
    tree->root->parent = NULL;
    tree->root->value = ZIP_RADIX_NO_VALUE;
    tree->count = 0;

    return tree;
}


void
radix_tree_delete(radix_tree_t *tree)
{
    ip_node_delete(tree->root, tree->value_delete_func);

    while (tree->free)
    {
        radix_node_t *free_node = tree->free;
        tree->free = tree->free->right;
        free(free_node);
    }

    free(tree);
}

void
radix_tree_walk(radix_tree_t *tree, value_func value_function)
{
    radix_tree_walk_helper(tree->root, value_function);
}

static int
trim_binary_subnet(const char *input_str, uint32_t *key)
{
    char *p = strchr(input_str, '/');
    if (!p)
        return -1;
    int i = 0;
    for (;*(p + 1 + i) != '\0'; i++)
        if (*(p + 1 + i) > '9' || *(p + 1 + i) < '0')
            return -1;

    int mask = atoi(p + 1);
    if (mask <= 0 || mask > 32)
        return -1;

    char bi_subnet[33];
    memcpy(bi_subnet, input_str, p-input_str);
    bi_subnet[p-input_str] = '\0';

    struct in_addr myin;
    int ret = inet_pton(AF_INET, bi_subnet, &myin);
    if (ret != 1)
        return -1;

    *key = ntohl(myin.s_addr);

    return mask;
}

result_t
radix_tree_insert_subnet(radix_tree_t *tree, const char *subnet, void* info)
{
    uint32_t key;
    int mask = trim_binary_subnet(subnet, &key);
    if (mask < 0)
        return FORMAT_ERROR;
    return radix_tree_insert_node(tree, key, mask, info);
}

result_t
radix_tree_insert_node(radix_tree_t *tree, uint32_t key,
                                           uint32_t mask, void *value)
{
    radix_node_t *node, *next;
    uint32_t bit = 0x80000000;
    node = next = tree->root;

    while (mask > 0)
    {
        if (key & bit)
            next = node->right;
        else
            next = node->left;

        if (next == NULL)
            break;

        bit >>= 1;
        node = next;
        --mask;
    }

    if (next)
    {
        if (node->value != ZIP_RADIX_NO_VALUE)
            return DUP_KEY;

        node->value = value;
        tree->count += 1;
        return SUCCEED;
    }

    while (mask > 0)
    {
        next = (radix_node_t *)radix_tree_alloc(tree);
        if (next == NULL) 
            return NO_MEMORY;
        
        next->right = NULL;
        next->left = NULL;
        next->parent = node;
        next->value = ZIP_RADIX_NO_VALUE;

        if (key & bit) 
            node->right = next;
        else 
            node->left = next;
        
        bit >>= 1;
        node = next;
        --mask;
    }

    node->value = value;
    tree->count += 1;

    return SUCCEED;
}

result_t
radix_tree_remove_subnet(radix_tree_t *tree, const char* subnet)
{
    uint32_t key;
    int mask = trim_binary_subnet(subnet, &key);
    if (mask <= 0)
        return FORMAT_ERROR;
    return radix_tree_delete_node(tree, key, mask);
}

result_t
radix_tree_delete_node(radix_tree_t *tree, uint32_t key, uint32_t mask)
{
    uint32_t bit = 0x80000000;
    radix_node_t *node = tree->root;

    while (node && mask > 0)
    {
        if (key & bit)
            node = node->right;
        else
            node = node->left;

        bit >>= 1;
        --mask;
    }

    if (node == NULL) 
        return FAILED;

    if (node->right || node->left)
    {
        if (node->value != ZIP_RADIX_NO_VALUE)
        {
            node->value = ZIP_RADIX_NO_VALUE;
            tree->count -= 1; 
            return SUCCEED;
        }

        return FAILED;
    }

    for ( ;; )
    {
        if (node->parent->right == node)
            node->parent->right = NULL;
        else
            node->parent->left = NULL;

        node->right = tree->free;
        tree->free = node;

        node = node->parent;

        if (node->right || node->left)
            break;

        if (node->value != ZIP_RADIX_NO_VALUE)
            break;

        if (node->parent == NULL)
            break;
    }

    tree->count -= 1; 
    return SUCCEED;
}

void *
radix_tree_find_str(radix_tree_t *tree, char *strip)
{
    struct in_addr myin;
    int ret = inet_pton(AF_INET, strip, &myin);
    if (ret != 1)
        return NULL;
    return radix_tree_find(tree, ntohl(myin.s_addr));
}

void *
radix_tree_find(radix_tree_t *tree, uint32_t key)
{
    uint32_t      bit = 0x80000000;
    void          *value = ZIP_RADIX_NO_VALUE;
    radix_node_t  *node = tree->root;

    while (node)
    {
        if (node->value != ZIP_RADIX_NO_VALUE)
            value = node->value;

        if (key & bit)
            node = node->right;

        else
            node = node->left;

        bit >>= 1;
    }

    return value;
}

static void *
radix_tree_alloc(radix_tree_t *tree)
{
    radix_node_t *ptr = NULL;

    if (tree->free)
    {
        ptr = tree->free;
        tree->free = tree->free->right;
        ptr->left = ptr->right = ptr->parent = NULL;
        ptr->value = ZIP_RADIX_NO_VALUE;
        return (void *)ptr;
    }

    ptr = (radix_node_t *)malloc(sizeof(radix_node_t));
    return (void *)ptr;
}

static void
ip_node_delete(radix_node_t *node, value_func del_func)
{
    if (node)
    {
        if (node->left)
            ip_node_delete(node->left, del_func);

        if (node->right)
            ip_node_delete(node->right, del_func);

        if (!NODE_IS_EMPTY(node) && del_func)
            del_func(node->value);

        free(node);
    }
}

static void
radix_tree_walk_helper(radix_node_t *node, value_func value_function)
{
    if (node->left)
        radix_tree_walk_helper(node->left, value_function);

    if (!NODE_IS_EMPTY(node))
        value_function(node->value);

    if (node->right)
        radix_tree_walk_helper(node->right, value_function);
}

uint32_t
radix_tree_get_ip_size(radix_tree_t *tree)
{
    return tree->count;
}
static void
radix_tree_print_helper(radix_node_t *node, uint32_t ip, int mask, traverse_func func)
{
    if (!NODE_IS_EMPTY(node))
    {
        uint32_t code = -1;
        code = code << (32 - mask);
        ip &= code;
        uint32_t temp = ip;
        temp = htonl(temp);
        char ip_addr[16];
        inet_ntop(AF_INET, &temp, ip_addr, 16);
        printf("ip is %s/%d--------", ip_addr, mask);
        if (func)
            func(node->value);
        printf("\n");
    }

    if (node->left)
    {
        mask++;
        radix_tree_print_helper(node->left, ip, mask, func);
        --mask;
    }

    if (node->right)
    {
        uint32_t bit = 0x80000000;
        bit >>= mask;
        ip |= bit;
        mask++;
        radix_tree_print_helper(node->right, ip, mask, func);
        --mask;
    }
}
void
radix_tree_print(radix_tree_t *tree, traverse_func func)
{
    radix_tree_print_helper(tree->root, 0, 0, func);
}

//static radix_node_t *
//radix_tree_get_leftest_node(radix_tree_t *tree)
//{
//    radix_node_t *node = tree->root;
//
//    while (node->left)
//        node = node->left;
//
//    return node;
//}
//
//static radix_node_t *
//ip_node_next(radix_node_t *node)
//{
//    radix_node_t *next = NULL;
//    if (node->right)
//    {
//        next = node->right;
//        while (next->left)
//            next = next->left;
//
//        return next;
//    }
//
//    radix_node_t *child = node;
//    radix_node_t *father = child->parent;
//    while (father)
//    {
//        if (child == father->left)
//            return father;
//
//        child = father;
//        father = father->parent;
//    }
//
//    return father;
//}
