#include "unity.h"
#include "main/controller.h"
#include "main/pinDefs.h"
#include "testControllerConfig.h"

#ifdef __cplusplus
extern "C" {
#endif

void includeControllerTests(void)
{
    // Dummy function to force discovery of unit tests by main test runner
}

ControllerConfig validConfig(void)
{
    ControllerConfig cfg {};
    cfg.dt = 1.0;
    cfg.refluxPumpConfig.pumpGPIO = GPIO_NUM_0;
    cfg.refluxPumpConfig.PWMChannel = LEDC_CHANNEL_0;
    cfg.refluxPumpConfig.timerChannel = LEDC_TIMER_0;
    cfg.prodPumpConfig.pumpGPIO = GPIO_NUM_0;
    cfg.prodPumpConfig.PWMChannel = LEDC_CHANNEL_0;
    cfg.prodPumpConfig.timerChannel = LEDC_TIMER_0;
    cfg.element1Pin = GPIO_NUM_0;
    cfg.element2Pin = GPIO_NUM_0;
    cfg.fanPin = GPIO_NUM_0;

    return cfg;
}

class ControllerUT
{
    public:
        static Pump& getRefluxPump(Controller& ctrl) { return ctrl._refluxPump; }
        static Pump& getProductPump(Controller& ctrl) { return ctrl._productPump; }
        static void setRefluxPump(Controller& ctrl, const Pump& pump) { ctrl._refluxPump = pump; }
        static void setProductPump(Controller& ctrl, const Pump& pump) { ctrl._productPump = pump; }
        static double getHysteresisUpperBound(Controller& ctrl) { return ctrl.HYSTERESIS_BOUND_UPPER; }
        static double getHysteresisLowerBound(Controller& ctrl) { return ctrl.HYSTERESIS_BOUND_LOWER; }
        static PBRet updateProductPump(Controller& ctrl, double T) { return ctrl._updateProductPump(T); }
        static PBRet initIO(Controller& ctrl, const ControllerConfig& cfg) { return ctrl._initIO(cfg); }
        static PBRet initPumps(Controller& ctrl, const PumpConfig& refluxCfg, const PumpConfig prodCfg) { return ctrl._initPumps(refluxCfg, prodCfg); }
        static PBRet updateRefluxPump(Controller& ctrl) { return ctrl._updateRefluxPump(); }
        static void setCurrentOutput(Controller& ctrl, double output) { ctrl._currentOutput = output; }
        static void setManualPumpSpeed(Controller& ctrl, const PumpSpeeds& pumpSpeeds) { ctrl._manualPumpSpeeds = pumpSpeeds; }
        static PBRet checkTemperatures(Controller& ctrl, const TemperatureData& currTemp) { return ctrl._checkTemperatures(currTemp); }
};

TEST_CASE("Constructor", "[Controller]")
{
    // Default constructor doesn't configure
    {
        ControllerConfig cfg {};
        Controller ctrl(1, 1024, 1, cfg);
        TEST_ASSERT_FALSE(ctrl.isConfigured());
    }

    // Valid construction
    {
        ControllerConfig cfg = validConfig();
        Controller ctrl(1, 1024, 1, cfg);
        TEST_ASSERT_TRUE(ctrl.isConfigured());
    }
}

TEST_CASE("checkInputs", "[Controller]")
{
    // Valid config
    {
        ControllerConfig cfg = validConfig();
        TEST_ASSERT_EQUAL(PBRet::SUCCESS, Controller::checkInputs(cfg));
    }

    // Invalid dt
    {
        ControllerConfig cfg = validConfig();
        cfg.dt = -1;
        TEST_ASSERT_EQUAL(PBRet::FAILURE, Controller::checkInputs(cfg));
    }

    // Invalid reflux pump config
    {
        ControllerConfig cfg = validConfig();
        cfg.refluxPumpConfig.pumpGPIO = static_cast<gpio_num_t> (GPIO_NUM_NC);
        TEST_ASSERT_EQUAL(PBRet::FAILURE, Controller::checkInputs(cfg));
    }

    // Invalid product pump config
    {
        ControllerConfig cfg = validConfig();
        cfg.prodPumpConfig.pumpGPIO = static_cast<gpio_num_t> (GPIO_NUM_NC);
        TEST_ASSERT_EQUAL(PBRet::FAILURE, Controller::checkInputs(cfg));
    }

    // Invalid fan pin
    {
        ControllerConfig cfg = validConfig();
        cfg.fanPin = static_cast<gpio_num_t> (GPIO_NUM_NC);
        TEST_ASSERT_EQUAL(PBRet::FAILURE, Controller::checkInputs(cfg));    
    }

    // Invalid LPElement pin
    {
        ControllerConfig cfg = validConfig();
        cfg.element1Pin = static_cast<gpio_num_t> (GPIO_NUM_NC);
        TEST_ASSERT_EQUAL(PBRet::FAILURE, Controller::checkInputs(cfg));
    }

    // Invalid HPElement pin
    {
        ControllerConfig cfg = validConfig();
        cfg.element2Pin = static_cast<gpio_num_t> (GPIO_NUM_NC);
        TEST_ASSERT_EQUAL(PBRet::FAILURE, Controller::checkInputs(cfg));
    }
}

TEST_CASE("InitIO", "[Controller]")
{
    Controller testCtrl(1, 1024, 1, validConfig());
    TEST_ASSERT_TRUE(testCtrl.isConfigured());

    // Valid config
    {
        const ControllerConfig validCfg = validConfig();
        TEST_ASSERT_EQUAL(PBRet::SUCCESS, ControllerUT::initIO(testCtrl, validCfg));
    }

    // Invalid fan pin
    {
        ControllerConfig validCfg = validConfig();
        validCfg.fanPin = static_cast<gpio_num_t> (GPIO_NUM_MAX);
        TEST_ASSERT_EQUAL(PBRet::FAILURE, ControllerUT::initIO(testCtrl, validCfg));
    }

    // Invalid LPElement pin
    {
        ControllerConfig validCfg = validConfig();
        validCfg.element1Pin = static_cast<gpio_num_t> (GPIO_NUM_MAX);
        TEST_ASSERT_EQUAL(PBRet::FAILURE, ControllerUT::initIO(testCtrl, validCfg));
    }

    // Invalid HPElement pin
    {
        ControllerConfig validCfg = validConfig();
        validCfg.element2Pin = static_cast<gpio_num_t> (GPIO_NUM_MAX);
        TEST_ASSERT_EQUAL(PBRet::FAILURE, ControllerUT::initIO(testCtrl, validCfg));
    }
}

TEST_CASE("InitPumps", "[Controller]")
{
    const PumpConfig validPumpConfig(GPIO_NUM_23, LEDC_CHANNEL_0, LEDC_TIMER_0);
    const PumpConfig invalidPumpConfig {};
    Controller testCtrl(1, 1024, 1, validConfig());
    TEST_ASSERT_TRUE(testCtrl.isConfigured());

    // Init valid pumps
    {
        TEST_ASSERT_EQUAL(PBRet::SUCCESS, ControllerUT::initPumps(testCtrl, validPumpConfig, validPumpConfig));
        TEST_ASSERT_EQUAL(PumpMode::Off, testCtrl.getRefluxPumpMode());
        TEST_ASSERT_EQUAL(PumpMode::Off, testCtrl.getProductPumpMode());
    }

    // Invalid reflux pump
    {
        TEST_ASSERT_EQUAL(PBRet::FAILURE, ControllerUT::initPumps(testCtrl, invalidPumpConfig, validPumpConfig));
    }

    // Invalid product pump
    {
        TEST_ASSERT_EQUAL(PBRet::FAILURE, ControllerUT::initPumps(testCtrl, validPumpConfig, invalidPumpConfig));
    }
}

TEST_CASE("updateRefluxPump", "[Controller]")
{

    // Unconfigured pump
    {
        Controller testCtrl(1, 1024, 1, validConfig());
        TEST_ASSERT_TRUE(testCtrl.isConfigured());
        PumpConfig invalidPumpCfg {};
        Pump invalidPump(invalidPumpCfg);
        ControllerUT::setRefluxPump(testCtrl, invalidPump);

        TEST_ASSERT_EQUAL(PBRet::FAILURE, ControllerUT::updateRefluxPump(testCtrl));
    }

    // Active control valid update
    {
        Controller testCtrl(1, 1024, 1, validConfig());
        TEST_ASSERT_TRUE(testCtrl.isConfigured());
        testCtrl.setRefluxPumpMode(PumpMode::ActiveControl);
        ControllerUT::setCurrentOutput(testCtrl, 256);
        Pump& refluxPump = ControllerUT::getRefluxPump(testCtrl);

        TEST_ASSERT_EQUAL(PBRet::SUCCESS, ControllerUT::updateRefluxPump(testCtrl));
        TEST_ASSERT_EQUAL(256, refluxPump.getPumpSpeed());
    }

    // Active control above max speed
    {
        Controller testCtrl(1, 1024, 1, validConfig());
        TEST_ASSERT_TRUE(testCtrl.isConfigured());
        testCtrl.setRefluxPumpMode(PumpMode::ActiveControl);
        ControllerUT::setCurrentOutput(testCtrl, Pump::PUMP_MAX_SPEED + 1);
        Pump& refluxPump = ControllerUT::getRefluxPump(testCtrl);

        TEST_ASSERT_EQUAL(PBRet::SUCCESS, ControllerUT::updateRefluxPump(testCtrl));
        TEST_ASSERT_EQUAL(Pump::PUMP_MAX_SPEED, refluxPump.getPumpSpeed());
    }

    // Manual control valid update
    {
        Controller testCtrl(1, 1024, 1, validConfig());
        TEST_ASSERT_TRUE(testCtrl.isConfigured());
        testCtrl.setRefluxPumpMode(PumpMode::ManualControl);
        Pump& refluxPump = ControllerUT::getRefluxPump(testCtrl);

        PumpSpeeds validSpeeds(256, 256);
        ControllerUT::setManualPumpSpeed(testCtrl, validSpeeds);

        TEST_ASSERT_EQUAL(PBRet::SUCCESS, ControllerUT::updateRefluxPump(testCtrl));
        TEST_ASSERT_EQUAL(256, refluxPump.getPumpSpeed());
    }

    // Manual control above max speed
    {
        Controller testCtrl(1, 1024, 1, validConfig());
        TEST_ASSERT_TRUE(testCtrl.isConfigured());
        testCtrl.setRefluxPumpMode(PumpMode::ManualControl);
        Pump& refluxPump = ControllerUT::getRefluxPump(testCtrl);

        PumpSpeeds invalidSpeeds(Pump::PUMP_MAX_SPEED + 1, Pump::PUMP_MAX_SPEED + 1);
        ControllerUT::setManualPumpSpeed(testCtrl, invalidSpeeds);

        TEST_ASSERT_EQUAL(PBRet::SUCCESS, ControllerUT::updateRefluxPump(testCtrl));
        TEST_ASSERT_EQUAL(Pump::PUMP_MAX_SPEED, refluxPump.getPumpSpeed());
    }

    // Turn pump off
    {
        Controller testCtrl(1, 1024, 1, validConfig());
        TEST_ASSERT_TRUE(testCtrl.isConfigured());
        testCtrl.setRefluxPumpMode(PumpMode::ActiveControl);
        ControllerUT::setCurrentOutput(testCtrl, 256);
        Pump& refluxPump = ControllerUT::getRefluxPump(testCtrl);

        // Set pump going
        TEST_ASSERT_EQUAL(PBRet::SUCCESS, ControllerUT::updateRefluxPump(testCtrl));
        TEST_ASSERT_EQUAL(256, refluxPump.getPumpSpeed());

        // Stop pump
        testCtrl.setRefluxPumpMode(PumpMode::Off);
        TEST_ASSERT_EQUAL(PBRet::SUCCESS, ControllerUT::updateRefluxPump(testCtrl));
        TEST_ASSERT_EQUAL(0, refluxPump.getPumpSpeed());
    }
}

TEST_CASE("updateProductPump", "[Controller]")
{
    Controller ctrl(1, 1024, 1, validConfig());
    TEST_ASSERT_TRUE(ctrl.isConfigured());
    const double hysteresisUpperBound = ControllerUT::getHysteresisUpperBound(ctrl);
    const double hysteresisLowerBound = ControllerUT::getHysteresisLowerBound(ctrl);
    const Pump& productPump = ControllerUT::getProductPump(ctrl);

    // Active control - Below lower bound rising
    TEST_ASSERT_EQUAL(PBRet::SUCCESS, ControllerUT::updateProductPump(ctrl, 0.0));
    TEST_ASSERT_EQUAL(Pump::PUMP_IDLE_SPEED, productPump.getPumpSpeed());

    // Active control - Above lower bound rising
    TEST_ASSERT_EQUAL(PBRet::SUCCESS, ControllerUT::updateProductPump(ctrl, hysteresisLowerBound + 1));
    TEST_ASSERT_EQUAL(Pump::PUMP_IDLE_SPEED, productPump.getPumpSpeed());

    // Active control - Above upper bound rising
    TEST_ASSERT_EQUAL(PBRet::SUCCESS, ControllerUT::updateProductPump(ctrl, hysteresisUpperBound + 1));
    TEST_ASSERT_EQUAL(Pump::PUMP_MAX_SPEED, productPump.getPumpSpeed());

    // Active control - Above lower bound falling
    TEST_ASSERT_EQUAL(PBRet::SUCCESS, ControllerUT::updateProductPump(ctrl, hysteresisLowerBound + 1));
    TEST_ASSERT_EQUAL(Pump::PUMP_MAX_SPEED, productPump.getPumpSpeed());

    // Active control - Below lower bound falling
    TEST_ASSERT_EQUAL(PBRet::SUCCESS, ControllerUT::updateProductPump(ctrl, 0.0));
    TEST_ASSERT_EQUAL(Pump::PUMP_IDLE_SPEED, productPump.getPumpSpeed());

    // Manual control - Valid speed
    ctrl.setProductPumpMode(PumpMode::ManualControl);
    PumpSpeeds validSpeeds(256, 256);
    ControllerUT::setManualPumpSpeed(ctrl, validSpeeds);
    TEST_ASSERT_EQUAL(PBRet::SUCCESS, ControllerUT::updateProductPump(ctrl, 0.0));
    TEST_ASSERT_EQUAL(256, productPump.getPumpSpeed());

    // Manual control - Above max speed
    PumpSpeeds invalidSpeeds(Pump::PUMP_MAX_SPEED + 1, Pump::PUMP_MAX_SPEED + 1);
    ControllerUT::setManualPumpSpeed(ctrl, invalidSpeeds);
    TEST_ASSERT_EQUAL(PBRet::SUCCESS, ControllerUT::updateProductPump(ctrl, 0.0));
    TEST_ASSERT_EQUAL(Pump::PUMP_MAX_SPEED, productPump.getPumpSpeed());

    // Turn pump off
    ctrl.setProductPumpMode(PumpMode::Off);
    TEST_ASSERT_EQUAL(PBRet::SUCCESS, ControllerUT::updateProductPump(ctrl, 0.0));
    TEST_ASSERT_EQUAL(0, productPump.getPumpSpeed());
}

TEST_CASE("checkTemperatures", "[Controller]")
{
    Controller ctrl(1, 1024, 1, validConfig());
    TEST_ASSERT_TRUE(ctrl.isConfigured());

    // Valid temperatures
    {
        TemperatureData validTemp(0.0, 0.0, 0.0, 0.0, 0.0);        // 0 is a valid temperature
        TEST_ASSERT_EQUAL(PBRet::SUCCESS, ControllerUT::checkTemperatures(ctrl, validTemp));
    }

    // Temperature above max control temp
    {
        TemperatureData invalidTemp(110.0, 0.0, 0.0, 0.0, 0.0);
        TEST_ASSERT_EQUAL(PBRet::FAILURE, ControllerUT::checkTemperatures(ctrl, invalidTemp));
    }

    // Temperature below min control temp
    {
        TemperatureData invalidTemp(-10.0, 0.0, 0.0, 0.0, 0.0);
        TEST_ASSERT_EQUAL(PBRet::FAILURE, ControllerUT::checkTemperatures(ctrl, invalidTemp));
    }

    // Temperature message is stale
    {
        TemperatureData validTemp(0.0, 0.0, 0.0, 0.0, 0.0);
        vTaskDelay(1250 / portTICK_PERIOD_MS);      // Wait for message to go stale
        TEST_ASSERT_EQUAL(PBRet::FAILURE, ControllerUT::checkTemperatures(ctrl, validTemp));
    }
}

TEST_CASE("loadFromJSONValid", "[Controller]")
{
    ControllerConfig testConfig {};
    cJSON* root = cJSON_Parse(ctrlConfig);
    TEST_ASSERT_NOT_EQUAL(root, nullptr);
 
    // Get config node
    cJSON* cfg = cJSON_GetObjectItem(root, "ControllerConfigValid");
    TEST_ASSERT_NOT_EQUAL(cfg, nullptr);

    TEST_ASSERT_EQUAL(PBRet::SUCCESS, Controller::loadFromJSON(testConfig, cfg));
}

TEST_CASE("loadFromJSONInvlid", "[Controller]")
{
    ControllerConfig testConfig {};
    cJSON* root = cJSON_Parse(ctrlConfig);
    TEST_ASSERT_NOT_EQUAL(root, nullptr);
 
    // Get config node
    cJSON* cfg = cJSON_GetObjectItem(root, "ControllerConfigInvalid");
    TEST_ASSERT_NOT_EQUAL(cfg, nullptr);

    TEST_ASSERT_EQUAL(PBRet::FAILURE, Controller::loadFromJSON(testConfig, cfg));
}

TEST_CASE("ControlTuning serialization/deserialization", "[Controller]")
{
    ControlTuning ctrlTuningIn(50.0, 25.0, 10.0, 75.0, 5.0);
    ControlTuning ctrlTuningOut {};

    // Test serialization
    std::string JSONStr {};
    TEST_ASSERT_EQUAL(PBRet::SUCCESS, ctrlTuningIn.serialize(JSONStr));

    // Test deserialization
    cJSON* root = cJSON_Parse(JSONStr.c_str());
    TEST_ASSERT_NOT_EQUAL(root, nullptr);
    ctrlTuningOut.deserialize(root);

    TEST_ASSERT_EQUAL_DOUBLE(ctrlTuningIn.getSetpoint(), ctrlTuningOut.getSetpoint());
    TEST_ASSERT_EQUAL_DOUBLE(ctrlTuningIn.getPGain(), ctrlTuningOut.getPGain());
    TEST_ASSERT_EQUAL_DOUBLE(ctrlTuningIn.getIGain(), ctrlTuningOut.getIGain());
    TEST_ASSERT_EQUAL_DOUBLE(ctrlTuningIn.getDGain(), ctrlTuningOut.getDGain());
    TEST_ASSERT_EQUAL_DOUBLE(ctrlTuningIn.getLPFCutoff(), ctrlTuningOut.getLPFCutoff());
}

TEST_CASE("ControlCommand serialization/deserialization", "[Controller]")
{
    ControlCommand ctrlCommandIn(ControllerState::ON, ControllerState::ON, ControllerState::ON);
    ControlCommand ctrlCommandOut {};

    // Test serialization
    std::string JSONStr {};
    TEST_ASSERT_EQUAL(PBRet::SUCCESS, ctrlCommandIn.serialize(JSONStr));

    // Test deserialization
    cJSON* root = cJSON_Parse(JSONStr.c_str());
    TEST_ASSERT_NOT_EQUAL(root, nullptr);
    ctrlCommandOut.deserialize(root);

    TEST_ASSERT_EQUAL(ctrlCommandIn.fanState, ctrlCommandOut.fanState);
    TEST_ASSERT_EQUAL(ctrlCommandIn.LPElementState, ctrlCommandOut.LPElementState);
    TEST_ASSERT_EQUAL(ctrlCommandIn.HPElementState, ctrlCommandOut.HPElementState);
}

#ifdef __cplusplus
}
#endif
