#include "unity.h"
#include "main/pump.h"
#include "testPumpConfig.h"

#ifdef __cplusplus
extern "C" {
#endif

void includePumpTests(void)
{
    // Dummy function to force discovery of unit tests by main test runner
}

static PumpConfig validConfig(void)
{
    PumpConfig cfg {};
    cfg.pumpGPIO = GPIO_NUM_0;
    cfg.PWMChannel = LEDC_CHANNEL_0;
    cfg.timerChannel = LEDC_TIMER_0;

    return cfg;
}

TEST_CASE("Constructor", "[Pump]")
{
    // Sucessful initialization
    {
        PumpConfig cfg(GPIO_NUM_23, LEDC_CHANNEL_0, LEDC_TIMER_0);
        Pump testPump(cfg);
        TEST_ASSERT_TRUE(testPump.isConfigured());
    }

    // Invalid config doesn't configure
    {
        PumpConfig cfg {};
        cfg.pumpGPIO = static_cast<gpio_num_t>(-1);
        Pump testPump(cfg);
        TEST_ASSERT_FALSE(testPump.isConfigured());
    }
}

TEST_CASE("Set Speed", "[Pump]")
{
    PumpConfig cfg(GPIO_NUM_23, LEDC_CHANNEL_0, LEDC_TIMER_0);
    Pump testPump(cfg);
    TEST_ASSERT_TRUE(testPump.isConfigured());

    // Valid speed
    {
        testPump.setSpeed(256);
        TEST_ASSERT_EQUAL_UINT16(256, testPump.getSpeed());
    }

    // Above maximum speed
    {
        testPump.setSpeed(1000);
        TEST_ASSERT_EQUAL_UINT16(Pump::PUMP_MAX_OUTPUT, testPump.getSpeed());
    }
    
    // Below miniumum speed
    {
        testPump.setSpeed(0);
        TEST_ASSERT_EQUAL_UINT16(Pump::PUMP_MIN_OUTPUT, testPump.getSpeed());
    }

    // Negative speec
    {
        testPump.setSpeed(-100);
        TEST_ASSERT_EQUAL_UINT16(Pump::PUMP_MIN_OUTPUT, testPump.getSpeed());
    }   
}

TEST_CASE("checkInputs", "[Pump]")
{
    // Default configuration invalid
    {
        PumpConfig cfg {};
        TEST_ASSERT_EQUAL(PBRet::FAILURE, Pump::checkInputs(cfg));
    }

    // Valid config
    {
        PumpConfig cfg = validConfig();
        TEST_ASSERT_EQUAL(PBRet::SUCCESS, Pump::checkInputs(cfg));
    }

    // Invalid pump GPIO
    {
        PumpConfig cfg = validConfig();
        cfg.pumpGPIO = static_cast<gpio_num_t>(-1);
        TEST_ASSERT_EQUAL(PBRet::FAILURE, Pump::checkInputs(cfg));
    }

    // Invalid PWM channe;
    {
        PumpConfig cfg = validConfig();
        cfg.PWMChannel = static_cast<ledc_channel_t>(-1);
        TEST_ASSERT_EQUAL(PBRet::FAILURE, Pump::checkInputs(cfg));
    }

    // Invalid timer channe;
    {
        PumpConfig cfg = validConfig();
        cfg.timerChannel = static_cast<ledc_timer_t>(-1);
        TEST_ASSERT_EQUAL(PBRet::FAILURE, Pump::checkInputs(cfg));
    }
}

TEST_CASE("loadFromJSON Valid", "[Pump]")
{
    PumpConfig testConfig {};
    cJSON* root = cJSON_Parse(pumpConfig);
    TEST_ASSERT_NOT_EQUAL(root, nullptr);
 
    // Get config node
    cJSON* cfg = cJSON_GetObjectItem(root, "ValidPump");
    TEST_ASSERT_NOT_EQUAL(cfg, nullptr);

    TEST_ASSERT_EQUAL(PBRet::SUCCESS, Pump::loadFromJSON(testConfig, cfg));
}

TEST_CASE("loadFromJSON Invalid", "[Pump]")
{
    PumpConfig testConfig {};
    cJSON* root = cJSON_Parse(pumpConfig);
    TEST_ASSERT_NOT_EQUAL(root, nullptr);
 
    // Get config node
    cJSON* cfg = cJSON_GetObjectItem(root, "InvalidPump");
    TEST_ASSERT_NOT_EQUAL(cfg, nullptr);

    TEST_ASSERT_EQUAL(PBRet::FAILURE, Pump::loadFromJSON(testConfig, cfg));
}

#ifdef __cplusplus
}
#endif