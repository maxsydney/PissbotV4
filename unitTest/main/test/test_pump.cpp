#include "unity.h"
#include "main/Pump.h"
#include "testPumpConfig.h"
#include "freertos/task.h"

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

TEST_CASE("updatePumpSpeed", "[Pump]")
{
    // Test public interface
    // NOTE: This test fails, ledc_get_duty always reports 0
    const PumpConfig cfg(GPIO_NUM_23, LEDC_CHANNEL_0, LEDC_TIMER_0);

    // Valid speed active
    {
        Pump testPump(cfg);
        TEST_ASSERT_TRUE(testPump.isConfigured());
        TEST_ASSERT_EQUAL(PBRet::SUCCESS, testPump.updatePumpSpeed(256));
        
        TEST_ASSERT_EQUAL(256, ledc_get_duty(LEDC_HIGH_SPEED_MODE, cfg.PWMChannel));
        TEST_ASSERT_EQUAL(256, testPump.getPumpSpeed());
    }

    // Above maximum speed
    {
        Pump testPump(cfg);
        TEST_ASSERT_TRUE(testPump.isConfigured());
        TEST_ASSERT_EQUAL(PBRet::SUCCESS, testPump.updatePumpSpeed(1000));
        
        TEST_ASSERT_EQUAL(Pump::PUMP_MAX_SPEED, ledc_get_duty(LEDC_HIGH_SPEED_MODE, cfg.PWMChannel));
        TEST_ASSERT_EQUAL(Pump::PUMP_MAX_SPEED, testPump.getPumpSpeed());
    }

    // Can't update speed on unconfigured pump
    {
        const PumpConfig invalidCfg {};
        Pump testPump(invalidCfg);
        TEST_ASSERT_FALSE(testPump.isConfigured());
        TEST_ASSERT_EQUAL(PBRet::FAILURE, testPump.updatePumpSpeed(1000));
    }
}

TEST_CASE("loadFromJSONValid", "[Pump]")
{
    PumpConfig testConfig {};
    cJSON* root = cJSON_Parse(pumpConfig);
    TEST_ASSERT_NOT_EQUAL(root, nullptr);
 
    // Get config node
    cJSON* cfg = cJSON_GetObjectItem(root, "ValidPump");
    TEST_ASSERT_NOT_EQUAL(cfg, nullptr);

    TEST_ASSERT_EQUAL(PBRet::SUCCESS, Pump::loadFromJSON(testConfig, cfg));
}

TEST_CASE("loadFromJSONInvalid", "[Pump]")
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
