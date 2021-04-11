#include "unity.h"
#include "main/DistillerManager.h"
#include "testDistillerManagerConfig.h"

#ifdef __cplusplus
extern "C" {
#endif

void includeDistillerManagerTests(void)
{
    // Dummy function to force discovery of unit tests by main test runner
}

TEST_CASE("loadFromJSONValid", "[DistillerManager]")
{
    DistillerConfig testConfig {};
    cJSON* root = cJSON_Parse(distillerManagerConfig);
    TEST_ASSERT_NOT_EQUAL(root, nullptr);
 
    // Get config node
    cJSON* cfg = cJSON_GetObjectItem(root, "DistillerConfigValid");
    TEST_ASSERT_NOT_EQUAL(cfg, nullptr);

    TEST_ASSERT_EQUAL(PBRet::SUCCESS, DistillerManager::loadFromJSON(testConfig, cfg));
}

TEST_CASE("loadFromJSONInvalid", "[DistillerManager]")
{
    DistillerConfig testConfig {};
    cJSON* root = cJSON_Parse(distillerManagerConfig);
    TEST_ASSERT_NOT_EQUAL(root, nullptr);
 
    // Get config node
    cJSON* cfg = cJSON_GetObjectItem(root, "DistillerConfigInvalid");
    TEST_ASSERT_NOT_EQUAL(cfg, nullptr);

    TEST_ASSERT_EQUAL(PBRet::FAILURE, DistillerManager::loadFromJSON(testConfig, cfg));
}

#ifdef __cplusplus
}
#endif