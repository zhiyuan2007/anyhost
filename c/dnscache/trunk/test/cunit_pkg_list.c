#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include <CUnit/Automated.h>
#include <CUnit/Console.h>

#include <stdlib.h>

#include "zip_pkg_list.h"
#include "zip_utils.h"

static void test_pkg_list()
{
    pkg_list_t *pkg_list = pkg_list_create(NULL);
    CU_ASSERT_TRUE(pkg_list != NULL);

    int i = 0;
    const int size = 30;
    for (; i < 10; i++)
    {
        char *raw_data = malloc(size);
        pkg_data_t *pkg_data = pkg_data_create_from(raw_data, size + 4 * i , i);
        CU_ASSERT_TRUE(pkg_data != NULL);
        CU_ASSERT_EQUAL(pkg_data_mem_size(pkg_data), size + 4 * i  + 12);
        pkg_list_insert(pkg_list, pkg_data);
    }

    CU_ASSERT_EQUAL(pkg_list_get_count(pkg_list), 10);

    i = 0;
    for (; i < 10; i++)
    {
        pkg_data_t *pkg_data = pkg_list_search(pkg_list, i);
        CU_ASSERT_TRUE(pkg_data != NULL);
        CU_ASSERT_TRUE(pkg_data_mem_size(pkg_data) == 30 + 4 * i + 12); 
        pkg_list_delete(pkg_list, pkg_data);
    }
    
    CU_ASSERT_EQUAL(pkg_list_get_count(pkg_list), 0);
    pkg_list_destroy(pkg_list);

}

static CU_TestInfo pkg_tests[] = {
    {"test_pkg_list", test_pkg_list},
    CU_TEST_INFO_NULL,
};

static CU_SuiteInfo  first_suit[] = {
    {"pkg_list_tests", NULL, NULL, pkg_tests},
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


