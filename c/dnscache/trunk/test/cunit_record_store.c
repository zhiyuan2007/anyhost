#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include <CUnit/Automated.h>
#include <CUnit/Console.h>

#include <stdlib.h>

#include "zip_lru_list.h"
#include "zip_pkg_list.h"
#include "dig_domain_rbtree.h"
#include "zip_record_store.h"
#include "dig_wire_name.h"
#include "dig_buffer.h"
#include "zip_utils.h"

static void test_record_store()
{
    record_store_t *record_store = record_store_create(1000);
    CU_ASSERT_TRUE(record_store != NULL);

    int i = 0;
    const int size = 30;
    for (;i < 10; i++)
    {
        wire_name_t *wire_name = wire_name_from_text("www.baidu.com");
        char *raw_data = malloc(size);
        memset(raw_data, 0, size);
        CU_ASSERT_TRUE(raw_data != NULL);
        record_store_insert_pkg(record_store, raw_data, size);
    }

    i = 0;
    for (;i < 10; i++)
    {
        wire_name_t *wire_name = wire_name_from_text("www.sina.com.cn");
        char *raw_data = malloc(size);
        memset(raw_data, 0, size);
        CU_ASSERT_TRUE(raw_data != NULL);
        record_store_insert_pkg(record_store, raw_data, size);
    }

    i = 0;
    for (;i < 10; i++)
    {
        wire_name_t *wire_name = wire_name_from_text("www.baidu.com");
        char *raw_data = malloc(size);
        CU_ASSERT_TRUE(raw_data != NULL);
        
        memset(raw_data, 0, size);
        record_store_insert_pkg(record_store, raw_data, size);
    }

    record_store_delete(record_store);

}

static CU_TestInfo record_store_tests[] = {
    {"test_record_store", test_record_store},
    CU_TEST_INFO_NULL,
};

static CU_SuiteInfo  first_suit[] = {
    {"record_store_tests", NULL, NULL, record_store_tests},
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


