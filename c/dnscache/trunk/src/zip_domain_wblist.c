#include "zip_domain_wblist.h"

typedef enum
{
    DOMAIN_WBLIST_SUCCESS = 0,
    DOMAIN_WBLIST_FAILED,
    DOMAIN_WBLIST_EXIST,
    DOMAIN_WBLIST_INVALID,
    DOMAIN_WBLIST_NO_MEM
}DOMAIN_RESULTS;
struct domain_wblist
{
    name_tree_t *name_tree; 
    uint32_t list_count;
    pthread_rwlock_t rw_lock;
};

const char *
domain_wblist_get_result(int type)
{
    switch(type)
    {
    case DOMAIN_WBLIST_SUCCESS:
        return "success";
    case DOMAIN_WBLIST_FAILED:
        return "domain name isn't exists";
    case DOMAIN_WBLIST_EXIST:
        return "domain name exists";
    case DOMAIN_WBLIST_INVALID:
        return "invalid domain name";
    default:
        return "unknown reason";
    }
}


domain_wblist_t *
domain_wblist_create()
{
     domain_wblist_t *domain_wblist = malloc(sizeof(domain_wblist_t)); 
     if (!domain_wblist)
         return NULL;
     domain_wblist->name_tree = name_tree_create(100, NULL);
     ASSERT(domain_wblist->name_tree, "create name tree failed in wblist");
     int ret = pthread_rwlock_init(&domain_wblist->rw_lock, NULL);
     ASSERT(ret == 0, "create rwlock failed in doamin wblist");

     domain_wblist->list_count = 0;

     return domain_wblist;
}

void
domain_wblist_destroy(domain_wblist_t *domain_wblist)
{
    name_tree_destroy(domain_wblist->name_tree);
    pthread_rwlock_destroy(&domain_wblist->rw_lock);
    free(domain_wblist);
}
void
domain_wblist_clear(domain_wblist_t  *domain_wblist)
{
    ASSERT(domain_wblist, "domain wblist can not be NULL");
    name_tree_clear(domain_wblist->name_tree);
}

int
domain_wblist_insert_from_text(domain_wblist_t *domain_wblist,
                               const char *domain_name)
{
    ASSERT(domain_wblist && domain_name, "domain wblist can not be NULL");
    wire_name_t *wire_name = wire_name_from_text(domain_name);
    if (!wire_name)
        return DOMAIN_WBLIST_INVALID;
    int ret = domain_wblist_insert_from_wire(domain_wblist, wire_name);
    wire_name_delete(wire_name);

    return ret;
}

int
domain_wblist_insert_from_wire(domain_wblist_t *domain_wblist,
                               const wire_name_t *wire_name)
{
    ASSERT(domain_wblist && wire_name, "parameter NULL in insert white");

    if (0 != domain_wblist_find_from_wire(domain_wblist, wire_name))
    {
        pthread_rwlock_wrlock(&domain_wblist->rw_lock);
        rbnode_t *rbnode = name_tree_insert(domain_wblist->name_tree, wire_name);
        if (!rbnode)
        {
            pthread_rwlock_unlock(&domain_wblist->rw_lock);
            return DOMAIN_WBLIST_NO_MEM;
        }
        pthread_rwlock_unlock(&domain_wblist->rw_lock);
        domain_wblist->list_count += 1;
    }
    else
    {
        return DOMAIN_WBLIST_EXIST;
    }
    return DOMAIN_WBLIST_SUCCESS;
}

int
domain_wblist_delete_from_wire(domain_wblist_t *domain_wblist,
                               wire_name_t *wire_name)
{
    ASSERT(domain_wblist && wire_name, "parameter NULL in delete");

    pthread_rwlock_wrlock(&domain_wblist->rw_lock);
    bool ret = name_tree_delete(domain_wblist->name_tree, wire_name);
    if (ret == true)
        domain_wblist->list_count -= 1;
    pthread_rwlock_unlock(&domain_wblist->rw_lock);

    if (ret == true)
        return DOMAIN_WBLIST_SUCCESS;
    else 
        return DOMAIN_WBLIST_FAILED;
}

int
domain_wblist_delete_from_text(domain_wblist_t *domain_wblist,
                                 const char *domain_name)
{
    ASSERT(domain_wblist && domain_name, "domain wblist can not be NULL");
    wire_name_t *wire_name = wire_name_from_text(domain_name);
    if (!wire_name)
        return DOMAIN_WBLIST_INVALID;
    int ret = domain_wblist_delete_from_wire(domain_wblist, wire_name);
    wire_name_delete(wire_name);

    return ret;
}

int
domain_wblist_find_from_wire(domain_wblist_t *domain_wblist,
                             const wire_name_t *wire_name)
{
    ASSERT(domain_wblist && wire_name, "parameter NULL in find");

    pthread_rwlock_rdlock(&domain_wblist->rw_lock);
    rbnode_t *rbnode = name_tree_find(domain_wblist->name_tree, wire_name);
    if (!rbnode)
    {
        pthread_rwlock_unlock(&domain_wblist->rw_lock);
        return 1;
    }
    pthread_rwlock_unlock(&domain_wblist->rw_lock);
    return 0;
}


int
domain_wblist_find_from_text(domain_wblist_t *domain_wblist,
                             const char *domain_name)
{
    ASSERT(domain_wblist && domain_name, "domain wblist can not be NULL");
    wire_name_t *wire_name = wire_name_from_text(domain_name);
    if (!wire_name)
        return 1;
    int result = domain_wblist_find_from_wire(domain_wblist, wire_name);
    wire_name_delete(wire_name);

    return result;
}

uint32_t
domain_wblist_get_count(domain_wblist_t *domain_wblist)
{
    ASSERT(domain_wblist, "domain wblist can not be NULL");
    return domain_wblist->list_count;
}
