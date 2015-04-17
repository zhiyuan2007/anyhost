#include "dig_domain_rbtree.h"
#include "zip_record_store.h"
#include "zip_pkg_list.h"
#include "zip_lru_list.h"
#include "dig_wire_name.h"
#include <pthread.h>

#define TOP_LIMIT_SIZE 1024
#define EACH_FREE_MEM 1024

struct record_store
{
    name_tree_t *domain_rbtree_;
    lru_list_t  *lru_list_;
    uint32_t total_mem_size;
    uint32_t mem_capacity;
    pthread_rwlock_t rw_lock;
};

void domain_tree_free_data(void *data)
{
     if (data)
         pkg_list_destroy((pkg_list_t *)data);
}

record_store_t *
record_store_create(uint32_t mem_capacity)
{
    record_store_t *record_store = malloc(sizeof(record_store_t));
    ASSERT(record_store, "no memory create record store");

    int ret = pthread_rwlock_init(&record_store->rw_lock, NULL);
    ASSERT(ret == 0, "create pthread rwlock failed");
   
    record_store->domain_rbtree_
            = name_tree_create(1024*1024, domain_tree_free_data);
    ASSERT(record_store->domain_rbtree_, "create domain tree failed");

    record_store->lru_list_ = lru_list_create();
    ASSERT(record_store->lru_list_, "lru list create failed");

    record_store->total_mem_size = 0;

    if (mem_capacity == 0)
        record_store->mem_capacity = TOP_LIMIT_SIZE;
    else
        record_store->mem_capacity = mem_capacity;

    return record_store;
}

void
record_store_delete(record_store_t *record_store)
{
    name_tree_destroy(record_store->domain_rbtree_);
    lru_list_delete(record_store->lru_list_);
    pthread_rwlock_destroy(&record_store->rw_lock);
    free(record_store);
}

void
record_store_set_capacity(record_store_t *record_store, uint32_t capacity)
{
    pthread_rwlock_wrlock(&record_store->rw_lock);
    record_store->mem_capacity = capacity;
    pthread_rwlock_unlock(&record_store->rw_lock);
}

static void
record_store_free_part_momery(record_store_t *record_store,
                              uint32_t required_mem_size)
{
    uint32_t freed_mem_size = 0;
    while(freed_mem_size < required_mem_size)
    {
        pkg_list_t *tail_list = lru_list_remove_tail_list(record_store->lru_list_);
        if (tail_list == NULL)
            break;

        freed_mem_size += pkg_list_mem_size(tail_list);
        wire_name_t *wire_name = (wire_name_t *)pkg_list_get_owner(tail_list);

        if (!name_tree_delete(record_store->domain_rbtree_,wire_name))
            ASSERT(0, "lru node isn't in the tree\n");
    };

    record_store->total_mem_size -= freed_mem_size;
}

void 
record_store_insert_pkg(record_store_t *record_store,
                        const char *raw_data,
                        uint16_t data_len)
{
    ASSERT(record_store && raw_data,"pointer can not be NULL");

    pthread_rwlock_wrlock(&record_store->rw_lock);

    if (record_store->total_mem_size > record_store->mem_capacity)
        record_store_free_part_momery(record_store, EACH_FREE_MEM);

    buffer_t buf;
    buffer_create_from(&buf, (void*)(raw_data + 12), data_len - 12);
    wire_name_t *wire_name = wire_name_from_wire(&buf);
    if (wire_name == NULL)
    {
        pthread_rwlock_unlock(&record_store->rw_lock);
        return;
    }

    uint16_t query_type = pkg_get_query_type(raw_data, data_len);
    pkg_data_t *pkg_data = pkg_data_create_from(raw_data, data_len, query_type);
    if (pkg_data == NULL)
    {
        wire_name_delete(wire_name);
        pthread_rwlock_unlock(&record_store->rw_lock);
        return;
    }

    rbnode_t *rbnode = name_tree_insert(record_store->domain_rbtree_, wire_name);
    ASSERT(rbnode, "domain rbtree insert failed");

    uint32_t before_insert_mem_size = 0;
    pkg_list_t *list = (pkg_list_t *)rbnode->value;
    if (list == NULL)
    {
        list = pkg_list_create(NULL);     
        pkg_list_set_owner(list, wire_name);
        rbnode->value = list;

        lru_list_insert(record_store->lru_list_, list);
    }
    else
    {
        before_insert_mem_size = pkg_list_mem_size(list);
        lru_list_revise(record_store->lru_list_, list);
        wire_name_delete(wire_name);
    }
        
    pkg_data_t *node = pkg_list_search(list, pkg_data_get_type(pkg_data)); 
    if (node)
        pkg_list_delete(list, node);

    pkg_list_insert(list, pkg_data);

    record_store->total_mem_size += (pkg_list_mem_size(list)
                                                    - before_insert_mem_size);

    pthread_rwlock_unlock(&record_store->rw_lock);

}

bool
record_store_get_pkg(record_store_t *record_store,
                     char *raw_data,
                     uint16_t *data_len)
{
    buffer_t buf;
    buffer_create_from(&buf, (void*)(raw_data + 12), *data_len - 12);
    wire_name_t *wire_name = wire_name_from_wire(&buf);
    if (!wire_name)
        return false;

    uint16_t query_type = pkg_get_query_type(raw_data, *data_len);

    pthread_rwlock_rdlock(&record_store->rw_lock);
    rbnode_t *rbnode = name_tree_find(record_store->domain_rbtree_, wire_name); 
    wire_name_delete(wire_name);
    if (NULL == rbnode)
    {
        pthread_rwlock_unlock(&record_store->rw_lock);
        return false;
    }
    
    pkg_list_t *list = (pkg_list_t *)rbnode->value;
    pkg_data_t *pkg_data = pkg_list_search(list, query_type);
    if (pkg_data == NULL)
    {
        pthread_rwlock_unlock(&record_store->rw_lock);
        return false;
    }

    uint16_t id = pkg_get_query_id(raw_data);
    *data_len = pkg_data_get_len(pkg_data);
    memcpy(raw_data, pkg_data_get_data(pkg_data), *data_len);

    pkg_set_response_id(raw_data, id);

    pthread_rwlock_unlock(&record_store->rw_lock);
     
    return true;
}



