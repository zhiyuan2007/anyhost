/*
 * cunit_test.c
 * Copyright (C) 2015 liuben <liuben@ubuntu>
 *
 * Distributed under terms of the MIT license.
 */

#include <stdio.h>
#include <string.h>
#include "CUnit/Basic.h"
#include "../log_name_tree.h"
#include "../dig_mem_pool.h"
#include "../log_view_stats.h"
#include "../log_view_tree.h"
#include "../dig_rb_tree.h"
#include "../log_heap.h"

/* Pointer to the file used by the tests. */
static name_tree_t *name_tree = NULL;
static view_tree_t *view_tree = NULL;
static view_stats_t *vs = NULL;
static inline
int name_compare(const void *value1, const void *value2)
{
    assert(value1 && value2);
    const char *name1 = ((view_tree_node_t *)value1)->name;
    const char *name2 = ((view_tree_node_t *)value2)->name;

    return strcmp(name1, name2);
}


/* The suite initialization function.
 *  * Opens the temporary file used by the tests.
 *   * Returns zero on success, non-zero otherwise.
 *    */
int init_suite1(void)
{
     name_tree = name_tree_create();
	 vs = view_stats_create("default");
	 view_tree = view_tree_create(name_compare);
	 return 0;
}

/* The suite cleanup function.
 *  * Closes the temporary file used by the tests.
 *   * Returns zero on success, non-zero otherwise.
 *    */
int clean_suite1(void)
{
	if (name_tree)
	    name_tree_destroy(name_tree);
	//if (vs)
	//	view_stats_destory(vs);
	if (view_tree)
		view_tree_destory(view_tree);
	return 0;
}

void *del_node_func(void *manager, void *data) {
	lru_list_t *lru = manager; 
	lru_node_t *node = data;
	lru_list_delete(lru,  node);
}

void test_name_tree_size(void)
{
    uint64_t initsize = sizeof(name_tree_t) + MAX_HEAP_NODE_NUM * sizeof(void *) + sizeof(heap_t) + 2 * mem_pool_struct_size();
	CU_ASSERT(initsize == name_tree_get_size(name_tree));

	char *domain = "www.baidu.com";
	name_tree_insert(name_tree, domain);

	uint64_t nodesize = sizeof(rbnode_t) + sizeof(node_value_t) + sizeof(lru_node_t);
	CU_ASSERT_EQUAL(initsize + nodesize, name_tree_get_size(name_tree));

	domain = "www.googlechina.com.cn";
	name_tree_insert(name_tree, domain);

	CU_ASSERT_EQUAL(initsize + 2 * nodesize, name_tree_get_size(name_tree));

	domain = "1234567890.1234567890.1234567890.cn";//exceed 31
	int n = name_tree_insert(name_tree, domain);
	CU_ASSERT_EQUAL(n, -1);

	CU_ASSERT_EQUAL(initsize + 2 * nodesize, name_tree_get_size(name_tree));

	domain = "1234567890.1234567890.123456.cn"; //31byte length
	n = name_tree_insert(name_tree, domain);
	CU_ASSERT_EQUAL(n, 0);
	CU_ASSERT_EQUAL(initsize + 3 * nodesize, name_tree_get_size(name_tree));

	domain = "www.googlechina.com.cn";
	name_tree_insert(name_tree, domain);
	CU_ASSERT_EQUAL(initsize + 3 * nodesize, name_tree_get_size(name_tree));

	domain = "www.baidu.com";
	name_tree_insert(name_tree, domain);
	CU_ASSERT_EQUAL(initsize + 3 * nodesize, name_tree_get_size(name_tree));

	domain = "www.baidu.com";
	name_tree_delete(name_tree, domain, del_node_func);
	printf("name tree size %lld , should %lld, count %lld\n", (initsize + 2*nodesize), name_tree_get_size(name_tree), name_tree->count);
	CU_ASSERT_EQUAL(initsize + 2 * nodesize, name_tree_get_size(name_tree));

	domain = "www.googlechina.com.cn";
	name_tree_delete(name_tree, domain, del_node_func);
	CU_ASSERT_EQUAL(initsize + 1 * nodesize, name_tree_get_size(name_tree));

}

void test_view_stats_test(void)
{
	uint64_t initsize = sizeof(view_stats_t);
	uint64_t nametree_size = name_tree_get_size(vs->name_tree);
	uint64_t iptree_size = name_tree_get_size(vs->ip_tree);
	CU_ASSERT_EQUAL(view_stats_get_size(vs), initsize + nametree_size + iptree_size);

	char *domain = "baidu.com";
	view_stats_insert_name(vs, domain);
	char *ip = "127.0.0.1";
	view_stats_insert_ip(vs, ip);

	nametree_size = name_tree_get_size(vs->name_tree);
	iptree_size = name_tree_get_size(vs->ip_tree);
	CU_ASSERT_EQUAL(view_stats_get_size(vs), initsize + nametree_size + iptree_size);

	int rcode = 0;
	view_stats_rcode_increment(vs, rcode);
	int rtype = 1;
	view_stats_rtype_increment(vs, rtype);
	CU_ASSERT_EQUAL(view_stats_get_size(vs), initsize + nametree_size + iptree_size);

	ip = "127.0.0.1";
	view_stats_insert_ip(vs, ip);

	CU_ASSERT_EQUAL(view_stats_get_size(vs), initsize + nametree_size + iptree_size);

}
void test_view_tree(void) {
     uint64_t initsize = sizeof(view_tree_t);
	 CU_ASSERT_EQUAL(view_tree_get_size(view_tree), initsize);

	 view_tree_insert(view_tree, "*$baidu.com", vs);

	 CU_ASSERT_EQUAL(view_tree_get_size(view_tree), initsize + view_stats_get_size(vs) + STATS_NODE_MAX_SIZE + sizeof(lru_node_t));
}

/* The main() function for setting up and running the tests.
 *  * Returns a CUE_SUCCESS on successful running, another
 *   * CUnit error code on failure.
 *    */
int main()
{
	CU_pSuite pSuite = NULL;

	/* initialize the CUnit test registry */
	if (CUE_SUCCESS != CU_initialize_registry())
		return CU_get_error();

	/* add a suite to the registry */
	pSuite = CU_add_suite("memory size test", init_suite1, clean_suite1);
	if (NULL == pSuite) {
		CU_cleanup_registry();
		return CU_get_error();
	}

	/* add the tests to the suite */
	/* NOTE - ORDER IS IMPORTANT - MUST TEST fread() AFTER fprintf() */
	if ((NULL == CU_add_test(pSuite, "test of init size and insert size)", test_name_tree_size)) ||
			(NULL == CU_add_test(pSuite, "test of close", test_view_stats_test)) ||
            (NULL == CU_add_test(pSuite, "test of close", test_view_tree))
			)
	{
		CU_cleanup_registry();
		return CU_get_error();
	}

	/* Run all tests using the CUnit Basic interface */
	CU_basic_set_mode(CU_BRM_VERBOSE);
	CU_basic_run_tests();
	CU_cleanup_registry();
	return CU_get_error();
}

