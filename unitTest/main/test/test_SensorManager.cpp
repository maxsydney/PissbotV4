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

TEST_CASE("checkInputs", "[SensorManager]")
{
    // Valid configuration
    {
        SensorManagerConfig cfg{};
        TEST_ASSERT_EQUAL(PBRet::FAILURE, SensorManager::checkInputs(cfg));
    }

    // Invalid dt
    {

    }

    // Invalid PBOneWire config
    {

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