#include "zip_ip_wblist.h"
#include "zip_utils.h"
#include <pthread.h>
#define USELESS_VALUE (void *)100
struct ip_wblist
{
    radix_tree_t *ip_tree;
    uint32_t list_count;
    pthread_rwlock_t rw_lock;
};

ip_wblist_t *
ip_wblist_create()
{
    ip_wblist_t *ip_wblist = malloc(sizeof(ip_wblist_t));
    if (ip_wblist)
    {
        ip_wblist->ip_tree = radix_tree_create(NULL);
        ASSERT(ip_wblist->ip_tree, "create radix tree failed");
        ip_wblist->list_count = 0;
        int ret = pthread_rwlock_init(&ip_wblist->rw_lock, NULL);
        ASSERT(ret == 0, "create rwlock failed in ip wblist");
    }
    return ip_wblist;
}

void
ip_wblist_destroy(ip_wblist_t *ip_wblist)
{
    radix_tree_delete(ip_wblist->ip_tree);
    pthread_rwlock_destroy(&ip_wblist->rw_lock);
    free(ip_wblist);
}

int
ip_wblist_insert_from_int(ip_wblist_t *ip_wblist, uint32_t key, uint32_t mask)
{
    pthread_rwlock_wrlock(&ip_wblist->rw_lock);
    int ret = radix_tree_insert_node(ip_wblist->ip_tree, key, mask, USELESS_VALUE);
    if (ret == 0)
        ip_wblist->list_count += 1;
    pthread_rwlock_unlock(&ip_wblist->rw_lock);
    return ret;
}
int
ip_wblist_insert_from_str(ip_wblist_t *ip_wblist, const char *str_ip)
{
    pthread_rwlock_wrlock(&ip_wblist->rw_lock);
    int ret = radix_tree_insert_subnet(ip_wblist->ip_tree, str_ip, USELESS_VALUE);
    if (ret == 0)
        ip_wblist->list_count += 1;
    pthread_rwlock_unlock(&ip_wblist->rw_lock);
    return ret;
}

int
ip_wblist_delete_from_int(ip_wblist_t *ip_wblist, uint32_t key, uint32_t mask)
{
    pthread_rwlock_wrlock(&ip_wblist->rw_lock);
    int ret = radix_tree_delete_node(ip_wblist->ip_tree, key, mask);
    if (ret == 0)
        ip_wblist->list_count -= 1;
    pthread_rwlock_unlock(&ip_wblist->rw_lock);
    return ret;
}

int
ip_wblist_delete_from_str(ip_wblist_t *ip_wblist, const char *str_ip)
{
    pthread_rwlock_wrlock(&ip_wblist->rw_lock);
    int ret = radix_tree_remove_subnet(ip_wblist->ip_tree, str_ip);
    if (ret == 0)
        ip_wblist->list_count -= 1;
    pthread_rwlock_unlock(&ip_wblist->rw_lock);
    return ret;
}

int
ip_wblist_find_from_int(ip_wblist_t *ip_wblist, uint32_t key)
{
    pthread_rwlock_rdlock(&ip_wblist->rw_lock);
    void *wb_value = radix_tree_find(ip_wblist->ip_tree, key);
    if (!wb_value)
    {
        pthread_rwlock_unlock(&ip_wblist->rw_lock);
        return 1;
    }
    pthread_rwlock_unlock(&ip_wblist->rw_lock);
    return 0; 
}

int
ip_wblist_find_from_str(ip_wblist_t *ip_wblist, const char *str_ip)
{
    pthread_rwlock_rdlock(&ip_wblist->rw_lock);
    void *wb_value = radix_tree_find_str(ip_wblist->ip_tree, str_ip);
    if (!wb_value)
    {
        pthread_rwlock_unlock(&ip_wblist->rw_lock);
        return 1;
    }
    pthread_rwlock_unlock(&ip_wblist->rw_lock);
    return 0; 
}

int
ip_wblist_clear(ip_wblist_t *ip_wblist)
{
    radix_tree_clear(ip_wblist->ip_tree); 
    ip_wblist->list_count = 0;
}
uint32_t
ip_wblist_get_count(ip_wblist_t *ip_wblist)
{
    return ip_wblist->list_count;
}
