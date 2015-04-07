#include <stdio.h>
#include <string.h>
#include "CUnit/Basic.h"

#include "libqueryhandler.h"
#include "libqueryhandler.c"
#include <microhttpd.h>

int initStatic(void)
{
    return 0;

}
 
int cleanStatic(void)
{
    return 0;
}

void testGetPort(void)
{
    int a = getPort();
    CU_ASSERT(a == DEFAULT_PORT);

}

int main()
{
    CU_pSuite pSuite = NULL;
 
    if (CU_initialize_registry() != CUE_SUCCESS)
	return CU_get_error();
 
    pSuite = CU_add_suite("StaticFuncs", initStatic, cleanStatic);
    if (pSuite == NULL) {
	CU_cleanup_registry();
	return CU_get_error();
    }
 
    if (CU_add_test(pSuite, "test of static functions", testGetPort) == NULL) {
	CU_cleanup_registry();
	return CU_get_error();
    }
 
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    /*
      CU_set_output_filename( "cunit_testall" );
      CU_list_tests_to_file();
      CU_automated_run_tests();
    */
    CU_cleanup_registry();
    return CU_get_error();
}
