#include "unity.h"
#include "main/SensorManager.h"
#include "testSensorManagerConfig.h"

#ifdef __cplusplus
extern "C" {
#endif

// class SensorManagerUT
// {
//     public:
        
// };

void includeSensorManagerTests(void)
{
    // Dummy function to force discovery of unit tests by main test runner
}

static SensorManagerConfig validConfig(void)
{
    SensorManagerConfig cfg {};
    cfg.dt = 0.1;
    cfg.oneWireConfig.oneWirePin = GPIO_NUM_0;
    cfg.oneWireConfig.tempSensorResolution = DS18B20_RESOLUTION_12_BIT;
    cfg.refluxFlowConfig.flowmeterPin = GPIO_NUM_0;
    cfg.refluxFlowConfig.kFactor = 1.0;
    cfg.productFlowConfig.flowmeterPin = GPIO_NUM_0;
    cfg.productFlowConfig.kFactor = 1.0;

    return cfg;
}

TEST_CASE("checkInputs", "[SensorManager]")
{
    // Default configuration invalid
    {
        SensorManagerConfig cfg {};
        TEST_ASSERT_EQUAL(PBRet::FAILURE, SensorManager::checkInputs(cfg));
    }

    // Valid config
    {
        SensorManagerConfig cfg = validConfig();
        TEST_ASSERT_EQUAL(PBRet::SUCCESS, SensorManager::checkInputs(cfg));
    }

    // Invalid dt
    {
        SensorManagerConfig cfg = validConfig();
        cfg.dt = -1.0;
        TEST_ASSERT_EQUAL(PBRet::FAILURE, SensorManager::checkInputs(cfg));
    }

    // Invalid PBOneWire config
    {
        SensorManagerConfig cfg = validConfig();
        cfg.oneWireConfig.oneWirePin = static_cast<gpio_num_t>(-1);
        TEST_ASSERT_EQUAL(PBRet::FAILURE, SensorManager::checkInputs(cfg));
    }

    // Invalid reflux flowmeter
    {
        SensorManagerConfig cfg = validConfig();
        cfg.refluxFlowConfig.flowmeterPin = static_cast<gpio_num_t>(-1);
        TEST_ASSERT_EQUAL(PBRet::FAILURE, SensorManager::checkInputs(cfg));
    }

    // Invalid product flowmeter
    {
        SensorManagerConfig cfg = validConfig();
        cfg.productFlowConfig.flowmeterPin = static_cast<gpio_num_t>(-1);
        TEST_ASSERT_EQUAL(PBRet::FAILURE, SensorManager::checkInputs(cfg));
    }
}

TEST_CASE("loadFromJSON Valid", "[SensorManager]")
{
    SensorManagerConfig testConfig {};
    cJSON* root = cJSON_Parse(sensorManagerConfig);
    TEST_ASSERT_NOT_EQUAL(root, nullptr);
 
    // Get config node
    cJSON* cfg = cJSON_GetObjectItem(root, "ValidSensorManagerConfig");
    TEST_ASSERT_NOT_EQUAL(cfg, nullptr);

    TEST_ASSERT_EQUAL(PBRet::SUCCESS, SensorManager::loadFromJSON(testConfig, cfg));
}

TEST_CASE("loadFromJSON Invalid", "[SensorManager]")
{
    SensorManagerConfig testConfig {};
    cJSON* root = cJSON_Parse(sensorManagerConfig);
    TEST_ASSERT_NOT_EQUAL(root, nullptr);
 
    // Get config node
    cJSON* cfg = cJSON_GetObjectItem(root, "InvalidSensorManagerConfig");
    TEST_ASSERT_NOT_EQUAL(cfg, nullptr);

    TEST_ASSERT_EQUAL(PBRet::FAILURE, SensorManager::loadFromJSON(testConfig, cfg));
}

#ifdef __cplusplus
}
#endif