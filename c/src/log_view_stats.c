#include <stdlib.h>
#include <string.h>
#include "log_view_stats.h"
#include "log_name_tree.h"
#include "log_utils.h"
#include "log_heap.h"
#include "statsmessage.pb-c.h"
char *RCODE[RCODE_MAX_NUM] = {
    "NOERROR",
    "FORMATERR",
    "SERVFAIL",
    "NXDOMAIN",
    "NOTIMP",
    "REFUSED",
    "OTHER"
};

char *RTYPE[RTYPE_MAX_NUM] = {
    "A",
    "MX",
    "AAAA",
    "CNAME",
    "NS",
    "SOA",
    "TXT",
	"SPF"
    "SRV",
    "DNAME",
    "NAPTR",
    "OTHER",
};

int rtype_index(const char *rtype)
{
    int i = 0 ;
    for( ; i < RTYPE_MAX_NUM; i++)
    {
        if (strcmp(rtype, RTYPE[i])== 0)
            return i;
    }
    return RTYPE_MAX_NUM-1;
}

int rcode_index(const char *rcode)
{
    int i = 0 ;
    for( ; i < RCODE_MAX_NUM; i++)
    {
        if (strcmp(rcode, RCODE[i])== 0)
            return i;
    }
    return RCODE_MAX_NUM -1;
}

view_stats_t *view_stats_create(const char *view_name)
{
	if (strlen(view_name) > MAX_VIEW_LENGTH )
		return NULL;

    view_stats_t *vs = malloc(sizeof(view_stats_t));
    if (NULL == vs)
        return NULL;
    vs->name_tree = name_tree_create();
    if (!vs->name_tree)
    {
        fprintf(stderr, "Error: create name tree\n");
        return NULL;
    }

    vs->ip_tree = name_tree_create();
    if (!vs->ip_tree)
    {
        fprintf(stderr, "Error: create ip tree\n");
        return NULL;
    }

    vs->qps = 0.0;
    vs->success_rate = 1.0;
    vs->count = 0;
    vs->last_count = 0;
    strcpy(vs->name, view_name);
    int i;
    for( i = 0; i < RCODE_MAX_NUM ; i++)
        vs->rcode[i] = 0;
    for( i = 0; i < RTYPE_MAX_NUM; i++)
        vs->rtype[i] = 0;
}

uint64_t view_stats_get_size(view_stats_t *vs)
{
	uint32_t initsize = sizeof(view_stats_t);
	uint32_t nametree_size = name_tree_get_size(vs->name_tree);
	uint32_t iptree_size = name_tree_get_size(vs->ip_tree);
	return initsize + nametree_size + iptree_size;
}
void view_stats_set_memsize(view_stats_t *vs, uint64_t expect_size){
	uint64_t current_size = view_stats_get_size(vs);
	printf("current_size %u, name node %u, ip node %u\n", current_size, vs->name_tree->count, vs->ip_tree->count);
	while (expect_size < current_size) {
		printf("delete name tree node due to memory bigger than expect\n");
		node_value_t *node = lru_list_remove_tail(vs->name_tree->lru);
		if (node == NULL) {
			break;
		}
		name_tree_delete(vs->name_tree, node->fqdn, NULL);
	    current_size = view_stats_get_size(vs);
	}
	while (expect_size < current_size) {
		printf("delete ip tree node due to memory bigger than expect\n");
		node_value_t *node = lru_list_remove_tail(vs->ip_tree->lru);
		if (node == NULL) {
			break;
		}
		name_tree_delete(vs->ip_tree, node->fqdn, NULL);
	    current_size = view_stats_get_size(vs);
	}
}

void view_stats_destory(view_stats_t *vs)
{
    ASSERT(vs, "empty pointer when destory view stats\n");
    name_tree_destroy(vs->name_tree);
    name_tree_destroy(vs->ip_tree);
    free(vs);
}

void view_stats_insert_name(view_stats_t *vs, const char *name)
{
    ASSERT(vs && name, "view stats or name is NULL when insert\n");
    name_tree_insert(vs->name_tree, name);
}

void view_stats_insert_ip(view_stats_t *vs, const char *ip)
{
    ASSERT(vs && ip, "view stats or name is NULL when insert\n");
    name_tree_insert(vs->ip_tree, ip);
}
void view_stats_rcode_increment(view_stats_t *vs, int rcode)
{
    ASSERT(vs, "view stats is NULL when insert\n");
    if (rcode > RCODE_MAX_NUM || rcode < 0)
        return;
    vs->rcode[rcode]++;
}

void view_stats_rtype_increment(view_stats_t *vs, int rtype)
{
    ASSERT(vs , "view stats is NULL when insert\n");
    if (rtype > RTYPE_MAX_NUM || rtype < 0)
        return;
    vs->rtype[rtype]++;
}

int min(int a, int b)
{
    return a < b ? a : b;
}
int max(int a, int b)
{
    return a > b ? a : b;
}

unsigned int _view_stats_result(name_tree_t *tree, char *key, int topn, char **buff)
{
    heap_t *copy_heap = heap_copy(tree->heap);
    if (NULL == copy_heap)
    {
        printf("empty heap\n");
        return 0;
    }
    heap_sort(copy_heap);
    int i;
            
    StatsReply reply = STATS_REPLY__INIT;

    reply.key = key;
    int _min = min(topn, copy_heap->len);
    reply.n_name = _min; 
    reply.n_count = _min; 
    char **pdata = malloc(sizeof (char *) * _min);
    memset(pdata, 0, sizeof(char *)*_min);
    int32_t *pcount = malloc(sizeof (int32_t) * _min);
    for(i = 0; i < reply.n_name; i++)
    {
        node_value_t *nv = (node_value_t *)(copy_heap->heap[i]);
        /*
         * new memory and copy if need absolutely right domain
        pdata[i] = malloc(sizeof(char)* (strlen(nv->fqdn) + 1));
        memcpy(pdata[i], nv->fqdn, (strlen(nv->fqdn) + 1));
        printf("malloc: addr of %d ptr is %p\n", i, pdata[i]);
        */
        pdata[i] = nv->fqdn;
        pcount[i]= nv->count;
    }
    
    reply.name = pdata;
    reply.count = pcount;
    unsigned int len = stats_reply__get_packed_size(&reply);
    char *result_ptr = malloc(sizeof(char) *len);
    stats_reply__pack(&reply, result_ptr);
    heap_destory(copy_heap);
    *buff = result_ptr;
    /* free mem if malloced above
    for (i=0; i<reply.n_name; i++)
    {
        printf("free addr of %d ptr is %p\n", i, pdata[i]);
        if (pdata[i] != NULL)
            free(pdata[i]);
    }
    */
    free(pcount);
    free(pdata);
    return len;
}
unsigned int view_stats_name_topn(view_stats_t *vs, int topn, char **buff)
{
	ASSERT(vs && vs->name_tree, "view stats or name tree is NULL");
    return _view_stats_result(vs->name_tree, "domaintopn", topn, buff);
}

unsigned int view_stats_ip_topn(view_stats_t *vs, int topn, char **buff)
{
	ASSERT(vs && vs->ip_tree, "view stats or ip tree is NULL");
    return _view_stats_result(vs->ip_tree, "iptopn", topn, buff);
}


unsigned int _view_stats_result1(int *array, char *key, char **buff)
{
    StatsReply reply = STATS_REPLY__INIT;
    reply.key = key;

    char **p_name = NULL;
    int need_length = 0;
    if (strcmp(key, "rcode") == 0)
    {
        p_name = RCODE;
        need_length = RCODE_MAX_NUM;
    }
    else
    {
        p_name = RTYPE;
        need_length = RTYPE_MAX_NUM;
    }

    int i = 0;
    int k = 0;
    char **pdata = malloc(sizeof (char *) * need_length);
    int32_t *pcount = malloc(sizeof (int32_t) * need_length);
    for (; i < need_length; i++)
    {
        if (array[i] == 0)
            continue;
        pdata[k] = p_name[i];
        pcount[k]= array[i];
        k++;
    }
    
    reply.n_name = k; 
    reply.n_count = k; 
    reply.name = pdata;
    reply.count = pcount;
    unsigned int len = stats_reply__get_packed_size(&reply);
    char *result_ptr = malloc(sizeof(char) *len);
    stats_reply__pack(&reply, result_ptr);
    *buff = result_ptr;
    free(pcount);
    free(pdata);
    return len;

}

unsigned int view_stats_get_rcode(view_stats_t *vs, char **buff)
{
    return _view_stats_result1(vs->rcode, "rcode", buff);
}

unsigned int view_stats_get_rtype(view_stats_t *vs, char **buff)
{
    return _view_stats_result1(vs->rtype, "rtype", buff);
}

unsigned int _view_stats_result2(float value, char *key, char **buff)
{
    StatsReply reply = STATS_REPLY__INIT;
    reply.key = key;
    reply.value = value;
    char tempbuf[16];
    sprintf(tempbuf, "%f", value);
    reply.maybe = tempbuf; 
    unsigned int len = stats_reply__get_packed_size(&reply);
    char *result_ptr = malloc(sizeof(char) *len);
    stats_reply__pack(&reply, result_ptr);
    *buff = result_ptr;
    return len;
}


unsigned int view_stats_get_qps(view_stats_t *vs, char **buff)
{
    return _view_stats_result2(vs->qps, "qps", buff);
}

unsigned int view_stats_get_success_rate(view_stats_t *vs, char **buff)
{
    return _view_stats_result2(vs->success_rate, "success_rate", buff);
}
