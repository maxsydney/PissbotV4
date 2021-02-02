#include "unity.h"
#include "main/pump.h"
#include "testPumpConfig.h"
#include "freertos/task.h"

#ifdef __cplusplus
extern "C" {
#endif

class PumpUT
{
    public:
        static PBRet setSpeed(Pump& pump, uint16_t pumpSpeed, PumpMode pumpMode) { return pump._setSpeed(pumpSpeed, pumpMode); }
        static PBRet drivePump(Pump& pump) { return pump._drivePump(); }
        static void setPumpSpeedActive(Pump& pump, uint16_t pumpSpeed) { pump._setSpeed (pumpSpeed, PumpMode::ActiveControl); }
        static void setPumpSpeedManual(Pump& pump, uint16_t pumpSpeed) { pump._setSpeed (pumpSpeed, PumpMode::ManualControl); }
};

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

TEST_CASE("Set Speed Active Mode", "[Pump]")
{
    PumpConfig cfg(GPIO_NUM_23, LEDC_CHANNEL_0, LEDC_TIMER_0);
    Pump testPump(cfg);
    testPump.setPumpMode(PumpMode::ActiveControl);      // Carry out tests in active mode
    TEST_ASSERT_TRUE(testPump.isConfigured());

    // Valid speed
    {
        TEST_ASSERT_EQUAL(PBRet::SUCCESS, PumpUT::setSpeed(testPump, 256, PumpMode::ActiveControl));
        TEST_ASSERT_EQUAL_UINT16(256, testPump.getPumpSpeed());
    }

    // Above maximum speed
    {
        TEST_ASSERT_EQUAL(PBRet::SUCCESS, PumpUT::setSpeed(testPump, 1000, PumpMode::ActiveControl));
        TEST_ASSERT_EQUAL_UINT16(Pump::PUMP_MAX_OUTPUT, testPump.getPumpSpeed());
    }
    
    // Below miniumum speed
    {
        TEST_ASSERT_EQUAL(PBRet::SUCCESS, PumpUT::setSpeed(testPump, 0, PumpMode::ActiveControl));
        TEST_ASSERT_EQUAL_UINT16(Pump::PUMP_MIN_OUTPUT, testPump.getPumpSpeed());
    }

    // Negative speec
    {
        TEST_ASSERT_EQUAL(PBRet::SUCCESS, PumpUT::setSpeed(testPump, -100, PumpMode::ActiveControl));
        TEST_ASSERT_EQUAL_UINT16(Pump::PUMP_MIN_OUTPUT, testPump.getPumpSpeed());
    }   
}

TEST_CASE("Set Speed Manual Mode", "[Pump]")
{
    PumpConfig cfg(GPIO_NUM_23, LEDC_CHANNEL_0, LEDC_TIMER_0);
    Pump testPump(cfg);
    testPump.setPumpMode(PumpMode::ManualControl);
    TEST_ASSERT_TRUE(testPump.isConfigured());

    // Valid speed
    {
        TEST_ASSERT_EQUAL(PBRet::SUCCESS, PumpUT::setSpeed(testPump, 256, PumpMode::ManualControl));
        TEST_ASSERT_EQUAL_UINT16(256, testPump.getPumpSpeed());
    }

    // Above maximum speed
    {
        TEST_ASSERT_EQUAL(PBRet::SUCCESS, PumpUT::setSpeed(testPump, 1000, PumpMode::ManualControl));
        TEST_ASSERT_EQUAL_UINT16(Pump::PUMP_MAX_OUTPUT, testPump.getPumpSpeed());
    }
    
    // Below miniumum speed
    {
        TEST_ASSERT_EQUAL(PBRet::SUCCESS, PumpUT::setSpeed(testPump, 0, PumpMode::ManualControl));
        TEST_ASSERT_EQUAL_UINT16(Pump::PUMP_MIN_OUTPUT, testPump.getPumpSpeed());
    }

    // Negative speec
    {
        TEST_ASSERT_EQUAL(PBRet::SUCCESS, PumpUT::setSpeed(testPump, -100, PumpMode::ManualControl));
        TEST_ASSERT_EQUAL_UINT16(Pump::PUMP_MIN_OUTPUT, testPump.getPumpSpeed());
    }   
}

TEST_CASE("Set Speed Off", "[Pump]")
{
    PumpConfig cfg(GPIO_NUM_23, LEDC_CHANNEL_0, LEDC_TIMER_0);
    Pump testPump(cfg);
    testPump.setPumpMode(PumpMode::Off);
    TEST_ASSERT_TRUE(testPump.isConfigured());

    // Valid speed
    {
        TEST_ASSERT_EQUAL(PBRet::FAILURE, PumpUT::setSpeed(testPump, 256, PumpMode::ManualControl));
        TEST_ASSERT_EQUAL_UINT16(0, testPump.getPumpSpeed());
    }

    // Above maximum speed
    {
        TEST_ASSERT_EQUAL(PBRet::FAILURE, PumpUT::setSpeed(testPump, 1000, PumpMode::ManualControl));
        TEST_ASSERT_EQUAL_UINT16(0, testPump.getPumpSpeed());
    }
    
    // Below miniumum speed
    {
        TEST_ASSERT_EQUAL(PBRet::FAILURE, PumpUT::setSpeed(testPump, 0, PumpMode::ManualControl));
        TEST_ASSERT_EQUAL_UINT16(0, testPump.getPumpSpeed());
    }

    // Negative speec
    {
        TEST_ASSERT_EQUAL(PBRet::FAILURE, PumpUT::setSpeed(testPump, -100, PumpMode::ManualControl));
        TEST_ASSERT_EQUAL_UINT16(0, testPump.getPumpSpeed());
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

TEST_CASE("drivePump", "[Pump]")
{
    const PumpConfig cfg(GPIO_NUM_23, LEDC_CHANNEL_0, LEDC_TIMER_0);

    // Valid speed active
    {
        Pump testPump(cfg);
        TEST_ASSERT_TRUE(testPump.isConfigured());
        testPump.setPumpMode(PumpMode::ActiveControl);
        PumpUT::setPumpSpeedActive(testPump, 200);

        TEST_ASSERT_EQUAL(PBRet::SUCCESS, PumpUT::drivePump(testPump));
        vTaskDelay(25 / portTICK_PERIOD_MS);
        TEST_ASSERT_EQUAL(200, ledc_get_duty(LEDC_HIGH_SPEED_MODE, cfg.PWMChannel));
    }

    // Valid speed manual
    {
        Pump testPump(cfg);
        TEST_ASSERT_TRUE(testPump.isConfigured());
        testPump.setPumpMode(PumpMode::ManualControl);
        PumpUT::setPumpSpeedManual(testPump, 200);
        
        TEST_ASSERT_EQUAL(PBRet::SUCCESS, PumpUT::drivePump(testPump));
        vTaskDelay(25 / portTICK_PERIOD_MS);
        TEST_ASSERT_EQUAL(200, ledc_get_duty(LEDC_HIGH_SPEED_MODE, cfg.PWMChannel));
    }

    // Above maximum speed active
    {
        Pump testPump(cfg);
        TEST_ASSERT_TRUE(testPump.isConfigured());
        testPump.setPumpMode(PumpMode::ActiveControl);
        PumpUT::setPumpSpeedActive(testPump, 1000);
        
        TEST_ASSERT_EQUAL(PBRet::SUCCESS, PumpUT::drivePump(testPump));
        vTaskDelay(25 / portTICK_PERIOD_MS);
        TEST_ASSERT_EQUAL(Pump::PUMP_MAX_OUTPUT, ledc_get_duty(LEDC_HIGH_SPEED_MODE, cfg.PWMChannel));
    }

    // Above maximum speed manual
    {
        Pump testPump(cfg);
        TEST_ASSERT_TRUE(testPump.isConfigured());
        testPump.setPumpMode(PumpMode::ManualControl);
        PumpUT::setPumpSpeedManual(testPump, 1000);
        
        TEST_ASSERT_EQUAL(PBRet::SUCCESS, PumpUT::drivePump(testPump));
        vTaskDelay(25 / portTICK_PERIOD_MS);
        TEST_ASSERT_EQUAL(Pump::PUMP_MAX_OUTPUT, ledc_get_duty(LEDC_HIGH_SPEED_MODE, cfg.PWMChannel));
    }

    // Below minimum speed active
    {
        Pump testPump(cfg);
        TEST_ASSERT_TRUE(testPump.isConfigured());
        testPump.setPumpMode(PumpMode::ActiveControl);
        PumpUT::setPumpSpeedActive(testPump, 0);
        
        TEST_ASSERT_EQUAL(PBRet::SUCCESS, PumpUT::drivePump(testPump));
        vTaskDelay(25 / portTICK_PERIOD_MS);
        TEST_ASSERT_EQUAL(Pump::PUMP_MIN_OUTPUT, ledc_get_duty(LEDC_HIGH_SPEED_MODE, cfg.PWMChannel));
    }

    // Below minimum speed manual
    {
        Pump testPump(cfg);
        TEST_ASSERT_TRUE(testPump.isConfigured());
        testPump.setPumpMode(PumpMode::ManualControl);
        PumpUT::setPumpSpeedManual(testPump, 0);
        
        TEST_ASSERT_EQUAL(PBRet::SUCCESS, PumpUT::drivePump(testPump));
        vTaskDelay(25 / portTICK_PERIOD_MS);
        TEST_ASSERT_EQUAL(Pump::PUMP_MIN_OUTPUT, ledc_get_duty(LEDC_HIGH_SPEED_MODE, cfg.PWMChannel));
    }
    
    // Negative speed active
    {
        Pump testPump(cfg);
        TEST_ASSERT_TRUE(testPump.isConfigured());
        testPump.setPumpMode(PumpMode::ActiveControl);
        PumpUT::setPumpSpeedActive(testPump, -100);
        
        TEST_ASSERT_EQUAL(PBRet::SUCCESS, PumpUT::drivePump(testPump));
        vTaskDelay(25 / portTICK_PERIOD_MS);
        TEST_ASSERT_EQUAL(Pump::PUMP_MIN_OUTPUT, ledc_get_duty(LEDC_HIGH_SPEED_MODE, cfg.PWMChannel));
    }

    // Negative speed manual
    {
        Pump testPump(cfg);
        TEST_ASSERT_TRUE(testPump.isConfigured());
        testPump.setPumpMode(PumpMode::ManualControl);
        PumpUT::setPumpSpeedManual(testPump, -100);
        
        TEST_ASSERT_EQUAL(PBRet::SUCCESS, PumpUT::drivePump(testPump));
        vTaskDelay(25 / portTICK_PERIOD_MS);
        TEST_ASSERT_EQUAL(Pump::PUMP_MIN_OUTPUT, ledc_get_duty(LEDC_HIGH_SPEED_MODE, cfg.PWMChannel));
    }

    // Pump off
    {
        Pump testPump(cfg);
        TEST_ASSERT_TRUE(testPump.isConfigured());
        testPump.setPumpMode(PumpMode::Off);
        
        TEST_ASSERT_EQUAL(PBRet::SUCCESS, PumpUT::drivePump(testPump));
        vTaskDelay(25 / portTICK_PERIOD_MS);
        TEST_ASSERT_EQUAL(0, ledc_get_duty(LEDC_HIGH_SPEED_MODE, cfg.PWMChannel));
    }
}

TEST_CASE("updatePumpActiveControl", "[Pump]")
{
    // Test public interface
    const PumpConfig cfg(GPIO_NUM_23, LEDC_CHANNEL_0, LEDC_TIMER_0);

    // Valid speed active
    {
        Pump testPump(cfg);
        TEST_ASSERT_TRUE(testPump.isConfigured());
        testPump.setPumpMode(PumpMode::ActiveControl);
        testPump.updatePumpActiveControl(200);
        vTaskDelay(25 / portTICK_PERIOD_MS);

        TEST_ASSERT_EQUAL(200, ledc_get_duty(LEDC_HIGH_SPEED_MODE, cfg.PWMChannel));
    }

    // Above maximum speed
    {
        Pump testPump(cfg);
        TEST_ASSERT_TRUE(testPump.isConfigured());
        testPump.setPumpMode(PumpMode::ActiveControl);
        testPump.updatePumpActiveControl(1000);
        vTaskDelay(25 / portTICK_PERIOD_MS);
        
        TEST_ASSERT_EQUAL(Pump::PUMP_MAX_OUTPUT, ledc_get_duty(LEDC_HIGH_SPEED_MODE, cfg.PWMChannel));
    }

    // Below minimum speed
    {
        Pump testPump(cfg);
        TEST_ASSERT_TRUE(testPump.isConfigured());
        testPump.setPumpMode(PumpMode::ActiveControl);
        testPump.updatePumpActiveControl(0);
        vTaskDelay(25 / portTICK_PERIOD_MS);
        
        TEST_ASSERT_EQUAL(Pump::PUMP_MIN_OUTPUT, ledc_get_duty(LEDC_HIGH_SPEED_MODE, cfg.PWMChannel));
    }
}

TEST_CASE("updatePumpManualControl", "[Pump]")
{
    // Test public interface
    const PumpConfig cfg(GPIO_NUM_23, LEDC_CHANNEL_0, LEDC_TIMER_0);

    // Valid speed active
    {
        Pump testPump(cfg);
        TEST_ASSERT_TRUE(testPump.isConfigured());
        testPump.setPumpMode(PumpMode::ManualControl);
        testPump.updatePumpManualControl(256);
        vTaskDelay(25 / portTICK_PERIOD_MS);
        
        TEST_ASSERT_EQUAL(256, ledc_get_duty(LEDC_HIGH_SPEED_MODE, cfg.PWMChannel));
    }

    // Above maximum speed
    {
        Pump testPump(cfg);
        TEST_ASSERT_TRUE(testPump.isConfigured());
        testPump.setPumpMode(PumpMode::ManualControl);
        testPump.updatePumpManualControl(1000);
        vTaskDelay(25 / portTICK_PERIOD_MS);
        
        TEST_ASSERT_EQUAL(Pump::PUMP_MAX_OUTPUT, ledc_get_duty(LEDC_HIGH_SPEED_MODE, cfg.PWMChannel));
    }

    // Below minimum speed
    {
        Pump testPump(cfg);
        TEST_ASSERT_TRUE(testPump.isConfigured());
        testPump.setPumpMode(PumpMode::ManualControl);
        testPump.updatePumpManualControl(0);
        vTaskDelay(25 / portTICK_PERIOD_MS);
        
        TEST_ASSERT_EQUAL(Pump::PUMP_MIN_OUTPUT, ledc_get_duty(LEDC_HIGH_SPEED_MODE, cfg.PWMChannel));
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