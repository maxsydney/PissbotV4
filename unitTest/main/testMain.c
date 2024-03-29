#include <stdio.h>
#include <string.h>
#include "unity.h"
#include "includeTestFiles.h"

static void print_banner(const char* text);

// This function calls a dummy function in each test file to 
// force the tests to be linked and discovered by unity
static void includeAllTests(void)
{
    includeThermoTests();
    includeUtilitiesTests();
    includeConnectionManagerTests();
    includeSensorManagerTests();
    includeOneWireBusTests();
    includeWebserverTests();
    includePumpTests();
    includeControllerTests();
    includeDistillerManagerTests();
    includeFlowmeterTests();
    includeSlowPWMTests();
    includeFilterTests();
    includeMessageServerTests();
}

void app_main(void)
{
    includeAllTests();      // Force discovery of all tests

    print_banner("Running all the registered tests");
    UNITY_BEGIN();
    unity_run_all_tests();
    UNITY_END();
    exit(0);
}

static void print_banner(const char* text)
{
    printf("\n#### %s #####\n\n", text);
}