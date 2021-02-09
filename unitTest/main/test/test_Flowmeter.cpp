#include "unity.h"
#include "main/Flowmeter.h"
#include "testFlowmeterConfig.h"

#ifdef __cplusplus
extern "C" {
#endif

void includeFlowmeterTests(void)
{
    // Dummy function to force discovery of unit tests by main test runner
}

static FlowmeterConfig validConfig(void)
{
    FlowmeterConfig cfg {};
    cfg.flowmeterPin = GPIO_NUM_0;
    cfg.kFactor = 1.0;

    return cfg;
}

TEST_CASE("checkInputs", "[Flowmeter]")
{
    // Default config invalid
    {
        FlowmeterConfig cfg {};
        TEST_ASSERT_EQUAL(PBRet::FAILURE, Flowmeter::checkInputs(cfg));
    }

    // Valid config
    {
        FlowmeterConfig cfg = validConfig();
        TEST_ASSERT_EQUAL(PBRet::SUCCESS, Flowmeter::checkInputs(cfg));
    }

    // Invalid flowmeter pin
    {
        FlowmeterConfig cfg = validConfig();
        cfg.flowmeterPin = static_cast<gpio_num_t>(-1);
        TEST_ASSERT_EQUAL(PBRet::FAILURE, Flowmeter::checkInputs(cfg));
    }

    // Invalid kFactor
    {
        FlowmeterConfig cfg = validConfig();
        cfg.kFactor = -1;
        TEST_ASSERT_EQUAL(PBRet::FAILURE, Flowmeter::checkInputs(cfg));
    }
}

TEST_CASE("loadFromJSON valid", "[Flowmeter]")
{
    FlowmeterConfig testConfig {};
    cJSON* root = cJSON_Parse(flowmeterTestConfig);
    TEST_ASSERT_NOT_EQUAL(root, nullptr);
 
    // Get config node
    cJSON* cfg = cJSON_GetObjectItem(root, "validFlowmeterConfig");
    TEST_ASSERT_NOT_EQUAL(cfg, nullptr);

    TEST_ASSERT_EQUAL(PBRet::SUCCESS, Flowmeter::loadFromJSON(testConfig, cfg));
}

TEST_CASE("loadFromJSON invalid", "[Flowmeter]")
{
    FlowmeterConfig testConfig {};
    cJSON* root = cJSON_Parse(flowmeterTestConfig);
    TEST_ASSERT_NOT_EQUAL(root, nullptr);
 
    // Get config node
    cJSON* cfg = cJSON_GetObjectItem(root, "invalidFlowmeterConfig");
    TEST_ASSERT_NOT_EQUAL(cfg, nullptr);

    TEST_ASSERT_EQUAL(PBRet::FAILURE, Flowmeter::loadFromJSON(testConfig, cfg));
}

#ifdef __cplusplus
}
#endif