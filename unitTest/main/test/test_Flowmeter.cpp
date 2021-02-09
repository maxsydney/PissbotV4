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

class FlowmeterUT
{
    public:
        static void setFreqCounter(Flowmeter& flow, uint16_t freqCount) { flow._freqCounter = freqCount; }
};

TEST_CASE("Constructor", "[Flowmeter]")
{
    // Default object not configured
    {
        Flowmeter flow {};
        TEST_ASSERT_FALSE(flow.isConfigured());
    }

    // Valid params
    {
        Flowmeter flow(validConfig());
        TEST_ASSERT_TRUE(flow.isConfigured());
    }

    // Invalid params
    {
        FlowmeterConfig cfg {};
        Flowmeter flow(cfg);
        TEST_ASSERT_FALSE(flow.isConfigured());
    }
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

TEST_CASE("readMassFlowrate", "[Flowmeter]")
{
    FlowmeterConfig cfg = validConfig();
    Flowmeter flow(cfg);
    TEST_ASSERT_TRUE(flow.isConfigured());
    double flowrate = 0.0;

    // Going back in time fails
    TEST_ASSERT_EQUAL(PBRet::FAILURE, flow.readMassFlowrate(-1, flowrate));

    // Valid flowmeter update
    FlowmeterUT::setFreqCounter(flow, 1);
    TEST_ASSERT_EQUAL(PBRet::SUCCESS, flow.readMassFlowrate(1e6, flowrate));
    TEST_ASSERT_EQUAL(1.0, flowrate);

    // Check internal time is updated
    FlowmeterUT::setFreqCounter(flow, 1);
    TEST_ASSERT_EQUAL(PBRet::SUCCESS, flow.readMassFlowrate(2e6, flowrate));
    TEST_ASSERT_EQUAL(1.0, flowrate);

    // Check internal flowrate is set
    TEST_ASSERT_EQUAL(1.0, flow.getFlowrate());
}

#ifdef __cplusplus
}
#endif