/*
 * cunit_test.c
 * Copyright (C) 2015 liuben <liuben@ubuntu>
 *
 * Distributed under terms of the MIT license.
 */

#include <stdio.h>
#include <string.h>
#include "CUnit/Basic.h"
#include "../zip_lru_list.h"

/* Pointer to the file used by the tests. */
static lru_list_t *list = NULL;
int init_suite1(void)
{
     list = lru_list_create();
	 return 0;
}

/* The suite cleanup function.
 *  * Closes the temporary file used by the tests.
 *   * Returns zero on success, non-zero otherwise.
 *    */
int clean_suite1(void)
{
	if (list)
		lru_list_destroy(list);
	return 0;
}

void test_lru_list(void)
{
	int i = 100;
	lru_node_t *node1 = lru_list_insert(list,  &i);

	int j = 200;
	lru_node_t *node2 = lru_list_insert(list,  &j);

	CU_ASSERT_EQUAL(lru_list_get_count(list), 2);

	void *rtn = lru_list_remove_tail(list);
	CU_ASSERT_EQUAL(*(int *)rtn, 100);
	CU_ASSERT_EQUAL(lru_list_get_count(list), 1);

	int k = 300;
	lru_node_t *node3 = lru_list_insert(list,  &k);
	CU_ASSERT_EQUAL(lru_list_get_count(list), 2);

	int p = 400;
	lru_node_t *node4 = lru_list_insert(list,  &p);
	CU_ASSERT_EQUAL(lru_list_get_count(list), 3);

	lru_list_move_to_first(list, node4);
	CU_ASSERT_EQUAL(*(int *)list->header->data, 400);

	lru_list_delete(list, node3);
	CU_ASSERT_EQUAL(lru_list_get_count(list), 2);

	lru_list_delete(list, node4);
	CU_ASSERT_EQUAL(lru_list_get_count(list), 1);

	lru_list_delete(list, node2);
	CU_ASSERT_EQUAL(lru_list_get_count(list), 0);
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
	pSuite = CU_add_suite("lru test", init_suite1, clean_suite1);
	if (NULL == pSuite) {
		CU_cleanup_registry();
		return CU_get_error();
	}

	/* add the tests to the suite */
	/* NOTE - ORDER IS IMPORTANT - MUST TEST fread() AFTER fprintf() */
	if ((NULL == CU_add_test(pSuite, "test lru list)", test_lru_list)))
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

