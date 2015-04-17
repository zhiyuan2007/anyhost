#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include <CUnit/Automated.h>
#include <CUnit/Console.h>

#include <stdlib.h>

#include "zip_lru_list.h"
#include "zip_pkg_list.h"
#include "zip_utils.h"

static void test_lru_list()
{
    lru_list_t *lru_list = lru_list_create();
    CU_ASSERT_TRUE(lru_list != NULL);
    
    int i = 0;
    for (; i < 10 ; i++)
    {
        pkg_list_t *pkg_list = pkg_list_create(NULL);
        CU_ASSERT_TRUE(pkg_list != NULL);
        lru_list_insert(lru_list, pkg_list);
    }

    CU_ASSERT_EQUAL(lru_list_get_count(lru_list), 10);
    int count = 0;
    while(1)
    {
        pkg_list_t *pkg_list = lru_list_remove_tail_list(lru_list);
        if (!pkg_list)
            break;
        CU_ASSERT_TRUE(pkg_list != NULL); 
        count += 1;

    }

    CU_ASSERT_TRUE(count == 10);
    CU_ASSERT_EQUAL(lru_list_get_count(lru_list), 0);

    lru_list_delete(lru_list);

}

static CU_TestInfo lru_tests[] = {
    {"test_lru_list", test_lru_list},
    CU_TEST_INFO_NULL,
};

static CU_SuiteInfo  first_suit[] = {
    {"lru_list_tests", NULL, NULL, lru_tests},
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


