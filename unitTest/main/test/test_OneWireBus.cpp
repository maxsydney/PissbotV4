#include "unity.h"
#include "main/OneWireBus.h"
#include "testOneWireBusConfig.h"

#ifdef __cplusplus
extern "C" {
#endif

void includeOneWireBusTests(void)
{
    // Dummy function to force discovery of unit tests by main test runner
}

static PBOneWireConfig validConfig(void)
{
    PBOneWireConfig cfg {};
    cfg.oneWirePin = GPIO_NUM_0;
    cfg.tempSensorResolution = DS18B20_RESOLUTION_12_BIT;

    return cfg;
}

TEST_CASE("checkInputs", "[OneWireBus]")
{
    // Default configuration invalid
    {
        PBOneWireConfig cfg {};
        TEST_ASSERT_EQUAL(PBRet::FAILURE, PBOneWire::checkInputs(cfg));
    }

    // Valid config
    {
        PBOneWireConfig cfg = validConfig();
        TEST_ASSERT_EQUAL(PBRet::SUCCESS, PBOneWire::checkInputs(cfg));
    }

    // Invalid OneWireBus GPIO
    {
        PBOneWireConfig cfg = validConfig();
        cfg.oneWirePin = static_cast<gpio_num_t>(-1);
        TEST_ASSERT_EQUAL(PBRet::FAILURE, PBOneWire::checkInputs(cfg));
    }

    // Invalid DS18B20 resolution
    {
        PBOneWireConfig cfg = validConfig();
        cfg.tempSensorResolution = DS18B20_RESOLUTION_INVALID;
        TEST_ASSERT_EQUAL(PBRet::FAILURE, PBOneWire::checkInputs(cfg));
    }
}

TEST_CASE("loadFromJSONValid", "[OneWireBus]")
{
    PBOneWireConfig testConfig {};
    cJSON* root = cJSON_Parse(OneWireBusConfig);
    TEST_ASSERT_NOT_EQUAL(root, nullptr);
 
    // Get config node
    cJSON* cfg = cJSON_GetObjectItem(root, "ValidOneWireConfig");
    TEST_ASSERT_NOT_EQUAL(cfg, nullptr);

    TEST_ASSERT_EQUAL(PBRet::SUCCESS, PBOneWire::loadFromJSON(testConfig, cfg));
}

TEST_CASE("loadFromJSONInvalid", "[OneWireBus]")
{
    PBOneWireConfig testConfig {};
    cJSON* root = cJSON_Parse(OneWireBusConfig);
    TEST_ASSERT_NOT_EQUAL(root, nullptr);
 
    // Get config node
    cJSON* cfg = cJSON_GetObjectItem(root, "InvalidOneWireConfig");
    TEST_ASSERT_NOT_EQUAL(cfg, nullptr);

    TEST_ASSERT_EQUAL(PBRet::FAILURE, PBOneWire::loadFromJSON(testConfig, cfg));
}

#ifdef __cplusplus
}
#endif