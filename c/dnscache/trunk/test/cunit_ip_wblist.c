#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include <CUnit/Automated.h>
#include <CUnit/Console.h>

#include <stdlib.h>
#include "zip_utils.h"
#include "zip_radix_tree.h"
#include "zip_ip_wblist.h"

static void test_ip_wblist()
{
    ip_wblist_t *ip_list = ip_wblist_create();
    CU_ASSERT_TRUE(ip_list != NULL);
    
    result_t ret = ip_wblist_insert_from_str(ip_list, "127.0.0.1/32");
    CU_ASSERT_TRUE(ret == 0);
    CU_ASSERT_EQUAL(ip_wblist_get_count(ip_list), 1);

    ret = ip_wblist_insert_from_str(ip_list, "127.0.0.1/24");
    CU_ASSERT_TRUE(ret == 0);
    CU_ASSERT_EQUAL(ip_wblist_get_count(ip_list), 2);

    ret = ip_wblist_delete_from_str(ip_list, "127.0.0.1/24");
    CU_ASSERT_TRUE(ret == 0);
    CU_ASSERT_EQUAL(ip_wblist_get_count(ip_list), 1);

    ret = ip_wblist_find_from_str(ip_list, "127.0.0.1");
    CU_ASSERT_TRUE(ret == 0);

    ip_wblist_clear(ip_list);
    CU_ASSERT_EQUAL(ip_wblist_get_count(ip_list), 0);
    ret = ip_wblist_insert_from_str(ip_list, "127.0.0.1/32");
    CU_ASSERT_TRUE(ret == 0);

    CU_ASSERT_EQUAL(ip_wblist_get_count(ip_list), 1);
    ret = ip_wblist_insert_from_str(ip_list, "127.0.0.1/24");
    CU_ASSERT_TRUE(ret == 0);
    CU_ASSERT_EQUAL(ip_wblist_get_count(ip_list), 2);

    ip_wblist_destroy(ip_list);


}
static void test_radix_tree()
{
    radix_tree_t *tree = radix_tree_create(NULL);
    CU_ASSERT_TRUE(tree != NULL);

    result_t ret = radix_tree_insert_subnet(tree, "127.0.0.1/32", (void *)100);
    CU_ASSERT_TRUE(ret  == 0);

    ret = radix_tree_insert_subnet(tree, "127.0.0.1/24", (void *)100);
    CU_ASSERT_TRUE(ret  == 0);

    ret = radix_tree_insert_subnet(tree, "127.0.0.1/24", (void *)100);
    CU_ASSERT_TRUE(ret  != 0);
    
    ret = radix_tree_insert_subnet(tree, "192.168.111.112/32", (void *)100);
    CU_ASSERT_TRUE(ret  == 0);
    
    ret = radix_tree_insert_subnet(tree, "192.168.221.0/24", (void *)100);
    CU_ASSERT_TRUE(ret  == 0);

    ret = radix_tree_insert_subnet(tree, "192.168.0.0/16", (void *)100);
    CU_ASSERT_TRUE(ret  == 0);

    radix_tree_print(tree);

    ret = radix_tree_remove_subnet(tree, "127.0.0.1/24");
    CU_ASSERT_TRUE(ret  == 0);

    radix_tree_clear(tree);
    
    ret = radix_tree_insert_subnet(tree, "127.0.0.1/24", (void *)100);
    CU_ASSERT_TRUE(ret  == 0);
    ret = radix_tree_remove_subnet(tree, "127.0.0.1/24");
    CU_ASSERT_TRUE(ret  == 0);
    radix_tree_delete(tree);
}
static CU_TestInfo ip_wblist_tests[] = {
    {"ip_wblist_test", test_ip_wblist},
    {"radix_tree_test", test_radix_tree},
    CU_TEST_INFO_NULL,
};

static CU_SuiteInfo  first_suit[] = {
    {"ip_wblist_tests", NULL, NULL, ip_wblist_tests},
    CU_SUITE_INFO_NULL,
};

int main()
{
    CU_BasicRunMode mode = CU_BRM_VERBOSE;
    CU_ErrorAction error_action = CUEA_FAIL;

    CU_initialize_registry();
    CU_register_suites(first_suit);
    CU_basic_set_mode(mode);
    CU_set_error_action(error_action);
    CU_basic_run_tests();
    CU_cleanup_registry();
    return 0;
}


