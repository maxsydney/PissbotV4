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
        static Pump& getProductPump(Controller& ctrl) { return ctrl._prodPump; }
        static double getHysteresisUpperBound(Controller& ctrl) { return ctrl.HYSTERESIS_BOUND_UPPER; }
        static double getHysteresisLowerBound(Controller& ctrl) { return ctrl.HYSTERESIS_BOUND_LOWER; }
        static PBRet handleProductPump(Controller& ctrl, double T) { return ctrl._handleProductPump(T); }
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

// TEST_CASE("Proportional Control Response", "[Controller]")
// {
//     // Setup proportional only control
//     const uint8_t freq = 5;
//     const ctrlParams_t ctrlParams = {0.0, 1.0, 0.0, 0.0};
//     const ctrlSettings_t ctrlSettings = {};
//     PumpConfig refluxPumpConfig(REFLUX_PUMP, LEDC_CHANNEL_0, LEDC_TIMER_0);
//     PumpConfig prodPumpConfig(PROD_PUMP, LEDC_CHANNEL_1, LEDC_TIMER_1);
//     Controller Ctrl = Controller(freq, ctrlParams, ctrlSettings, refluxPumpConfig,  prodPumpConfig, FAN_SWITCH, ELEMENT_2, ELEMENT_2);

//     Ctrl.updatePumpSpeed(25.0);
//     TEST_ASSERT_EQUAL_INT16(25, Ctrl.getRefluxPumpSpeed());

//     Ctrl.updatePumpSpeed(50.0);
//     TEST_ASSERT_EQUAL_INT16(50, Ctrl.getRefluxPumpSpeed());

//     Ctrl.updatePumpSpeed(75.0);
//     TEST_ASSERT_EQUAL_INT16(75, Ctrl.getRefluxPumpSpeed());
// }

// TEST_CASE("Integral Control Response", "[Controller]")
// {
//     // Setup integral only control
//     const uint8_t freq = 1;
//     const ctrlParams_t ctrlParams = {0.0, 0.0, 1.0, 0.0};
//     const ctrlSettings_t ctrlSettings = {};
//     PumpConfig refluxPumpConfig(REFLUX_PUMP, LEDC_CHANNEL_0, LEDC_TIMER_0);
//     PumpConfig prodPumpConfig(PROD_PUMP, LEDC_CHANNEL_1, LEDC_TIMER_1);
//     Controller Ctrl = Controller(freq, ctrlParams, ctrlSettings, refluxPumpConfig,  prodPumpConfig, FAN_SWITCH, ELEMENT_2, ELEMENT_2);

//     Ctrl.updatePumpSpeed(25.0);
//     TEST_ASSERT_EQUAL_INT16(25, Ctrl.getRefluxPumpSpeed());

//     Ctrl.updatePumpSpeed(25.0);
//     TEST_ASSERT_EQUAL_INT16(50, Ctrl.getRefluxPumpSpeed());

//     Ctrl.updatePumpSpeed(25.0);
//     TEST_ASSERT_EQUAL_INT16(75, Ctrl.getRefluxPumpSpeed());
// }

// TEST_CASE("Derivative Control Response", "[Controller]")
// {
//     // Setup derivative only control
//     const uint8_t freq = 1;
//     const ctrlParams_t ctrlParams = {0.0, 0.0, 0.0, 1.0};
//     const ctrlSettings_t ctrlSettings = {};
//     PumpConfig refluxPumpConfig(REFLUX_PUMP, LEDC_CHANNEL_0, LEDC_TIMER_0);
//     PumpConfig prodPumpConfig(PROD_PUMP, LEDC_CHANNEL_1, LEDC_TIMER_1);
//     Controller Ctrl = Controller(freq, ctrlParams, ctrlSettings, refluxPumpConfig,  prodPumpConfig, FAN_SWITCH, ELEMENT_2, ELEMENT_2);

//     Ctrl.updatePumpSpeed(50.0);
//     TEST_ASSERT_EQUAL_INT16(50, Ctrl.getRefluxPumpSpeed());

//     Ctrl.updatePumpSpeed(75.0);
//     TEST_ASSERT_EQUAL_INT16(25, Ctrl.getRefluxPumpSpeed());

//     Ctrl.updatePumpSpeed(175);
//     TEST_ASSERT_EQUAL_INT16(100, Ctrl.getRefluxPumpSpeed());
// }

// TEST_CASE("PID control Response", "[Controller]")
// {
//     // Setup full PID control
//     const uint8_t freq = 1;
//     const ctrlParams_t ctrlParams = {0.0, 1.0, 1.0, 1.0};
//     const ctrlSettings_t ctrlSettings = {};
//     PumpConfig refluxPumpConfig(REFLUX_PUMP, LEDC_CHANNEL_0, LEDC_TIMER_0);
//     PumpConfig prodPumpConfig(PROD_PUMP, LEDC_CHANNEL_1, LEDC_TIMER_1);
//     Controller Ctrl = Controller(freq, ctrlParams, ctrlSettings, refluxPumpConfig,  prodPumpConfig, FAN_SWITCH, ELEMENT_2, ELEMENT_2);

//     // Proportional is 25, integral is 25, derivative is 25
//     Ctrl.updatePumpSpeed(25.0);
//     TEST_ASSERT_EQUAL_INT16(75, Ctrl.getRefluxPumpSpeed());

//     // Proportional is 25, integral is 50, derivative is 0
//     Ctrl.updatePumpSpeed(25.0);
//     TEST_ASSERT_EQUAL_INT16(75, Ctrl.getRefluxPumpSpeed());

//     // Proportional is 50, integral is 100, derivative is 25
//     Ctrl.updatePumpSpeed(50);
//     TEST_ASSERT_EQUAL_INT16(175, Ctrl.getRefluxPumpSpeed());

//     // Proportional is 50, integral is 150, derivative is 0
//     Ctrl.updatePumpSpeed(50);
//     TEST_ASSERT_EQUAL_INT16(200, Ctrl.getRefluxPumpSpeed());

//     // Proportional is 100, integral is 250, derivative is 50
//     Ctrl.updatePumpSpeed(100);
//     TEST_ASSERT_EQUAL_INT16(400, Ctrl.getRefluxPumpSpeed());

//     // Proportional is 50, integral is 300, derivative is -50
//     Ctrl.updatePumpSpeed(50);
//     TEST_ASSERT_EQUAL_INT16(300, Ctrl.getRefluxPumpSpeed());

//     // Proportional is -50, integral is 250, derivative is -100
//     Ctrl.updatePumpSpeed(-50);
//     TEST_ASSERT_EQUAL_INT16(100, Ctrl.getRefluxPumpSpeed());

//     // Proportional is -25, integral is 225, derivative is 25
//     Ctrl.updatePumpSpeed(-25);
//     TEST_ASSERT_EQUAL_INT16(225, Ctrl.getRefluxPumpSpeed());

//     // Proportional is -25, integral is 200, derivative is 0
//     Ctrl.updatePumpSpeed(-25);
//     TEST_ASSERT_EQUAL_INT16(175, Ctrl.getRefluxPumpSpeed());

//     // Proportional is -25, integral is 175, derivative is 0
//     Ctrl.updatePumpSpeed(-25);
//     TEST_ASSERT_EQUAL_INT16(150, Ctrl.getRefluxPumpSpeed());
// }

// TEST_CASE("Output Saturation", "[Controller]")
// {
//     // Setup proportional only control
//     const uint8_t freq = 1;
//     const ctrlParams_t ctrlParams = {0.0, 1.0, 0.0, 0.0};
//     const ctrlSettings_t ctrlSettings = {};
//     PumpConfig refluxPumpConfig(REFLUX_PUMP, LEDC_CHANNEL_0, LEDC_TIMER_0);
//     PumpConfig prodPumpConfig(PROD_PUMP, LEDC_CHANNEL_1, LEDC_TIMER_1);
//     Controller Ctrl = Controller(freq, ctrlParams, ctrlSettings, refluxPumpConfig,  prodPumpConfig, FAN_SWITCH, ELEMENT_2, ELEMENT_2);

//     // Check minimum speed
//     Ctrl.updatePumpSpeed(0);
//     TEST_ASSERT_EQUAL_INT16(Pump::PUMP_MIN_OUTPUT, Ctrl.getRefluxPumpSpeed());
    
//     // Check maximum speed
//     Ctrl.updatePumpSpeed(10000);
//     TEST_ASSERT_EQUAL_INT16(Pump::PUMP_MAX_OUTPUT, Ctrl.getRefluxPumpSpeed());

//     // Check normal speed
//     Ctrl.updatePumpSpeed(75.0);
//     TEST_ASSERT_EQUAL_INT16(75, Ctrl.getRefluxPumpSpeed());
// }

// TEST_CASE("Auto Product Condensor", "[Controller]")
// {
//     const uint8_t freq = 1;
//     const ctrlParams_t ctrlParams = {0.0, 1.0, 0.0, 0.0};
//     const ctrlSettings_t ctrlSettings = {};
//     PumpConfig refluxPumpConfig(REFLUX_PUMP, LEDC_CHANNEL_0, LEDC_TIMER_0);
//     PumpConfig prodPumpConfig(PROD_PUMP, LEDC_CHANNEL_1, LEDC_TIMER_1);
//     Controller Ctrl = Controller(freq, ctrlParams, ctrlSettings, refluxPumpConfig,  prodPumpConfig, FAN_SWITCH, ELEMENT_2, ELEMENT_2);

//     TEST_ASSERT_EQUAL_INT16(Pump::PUMP_MIN_OUTPUT, Ctrl.getProductPumpSpeed());

//     // Set temp above 60 degrees
//     Ctrl.updatePumpSpeed(75);
//     TEST_ASSERT_EQUAL_INT16(Pump::FLUSH_SPEED, Ctrl.getProductPumpSpeed());
// }

// TEST_CASE("Set Controller Settings", "[Controller]")
// {
//     const uint8_t freq = 1;
//     const ctrlParams_t ctrlParams = {1.0, 1.0, 1.0, 1.0};
//     ctrlSettings_t ctrlSettingsIn = {};
//     ctrlSettings_t ctrlSettingsOut = {};
//     PumpConfig refluxPumpConfig(REFLUX_PUMP, LEDC_CHANNEL_0, LEDC_TIMER_0);
//     PumpConfig prodPumpConfig(PROD_PUMP, LEDC_CHANNEL_1, LEDC_TIMER_1);
//     Controller Ctrl = Controller(freq, ctrlParams, ctrlSettingsIn, refluxPumpConfig,  prodPumpConfig, FAN_SWITCH, ELEMENT_2, ELEMENT_2);

//     // Check all settings as defaults
//     ctrlSettingsOut = Ctrl.getControllerSettings();
//     TEST_ASSERT_EQUAL_INT(0, ctrlSettingsOut.fanState);
//     TEST_ASSERT_EQUAL_INT(0, ctrlSettingsOut.flush);
//     TEST_ASSERT_EQUAL_INT(0, ctrlSettingsOut.elementLow);
//     TEST_ASSERT_EQUAL_INT(0, ctrlSettingsOut.elementHigh);
//     TEST_ASSERT_EQUAL_INT(0, ctrlSettingsOut.prodCondensor);
//     TEST_ASSERT_TRUE(Ctrl.getRefluxPumpMode() == PumpMode::ACTIVE);
//     TEST_ASSERT_TRUE(Ctrl.getProductPumpMode() == PumpMode::ACTIVE);

//     // Set fan on
//     ctrlSettingsIn.fanState = 1;
//     Ctrl.setControllerSettings(ctrlSettingsIn);
//     ctrlSettingsOut = Ctrl.getControllerSettings();
//     TEST_ASSERT_EQUAL_INT(1, ctrlSettingsOut.fanState);

//     // Set flush
//     ctrlSettingsIn.flush = 1;
//     Ctrl.setControllerSettings(ctrlSettingsIn);
//     ctrlSettingsOut = Ctrl.getControllerSettings();
//     TEST_ASSERT_EQUAL_INT(1, ctrlSettingsOut.flush);
//     TEST_ASSERT_EQUAL_INT16(Pump::FLUSH_SPEED, Ctrl.getRefluxPumpSpeed());
//     TEST_ASSERT_EQUAL_INT16(Pump::FLUSH_SPEED, Ctrl.getProductPumpSpeed());
//     TEST_ASSERT_TRUE(Ctrl.getRefluxPumpMode() == PumpMode::FIXED);
//     TEST_ASSERT_TRUE(Ctrl.getProductPumpMode() == PumpMode::FIXED);

//     // Set elementLow
//     ctrlSettingsIn.elementLow = 1;
//     Ctrl.setControllerSettings(ctrlSettingsIn);
//     ctrlSettingsOut = Ctrl.getControllerSettings();
//     TEST_ASSERT_EQUAL_INT(1, ctrlSettingsOut.elementLow);

//     // Set elementHigh
//     ctrlSettingsIn.elementHigh = 1;
//     Ctrl.setControllerSettings(ctrlSettingsIn);
//     ctrlSettingsOut = Ctrl.getControllerSettings();
//     TEST_ASSERT_EQUAL_INT(1, ctrlSettingsOut.elementHigh);

//     // Set productCondensor
//     ctrlSettingsIn.prodCondensor = 1;
//     Ctrl.setControllerSettings(ctrlSettingsIn);
//     ctrlSettingsOut = Ctrl.getControllerSettings();
//     TEST_ASSERT_EQUAL_INT(1, ctrlSettingsOut.prodCondensor);
//     TEST_ASSERT_EQUAL_INT16(Pump::FLUSH_SPEED, Ctrl.getProductPumpSpeed());
//     TEST_ASSERT_TRUE(Ctrl.getProductPumpMode() == PumpMode::FIXED);
// }

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

TEST_CASE("handleProductPump", "[Controller]")
{
    Controller ctrl(1, 1024, 1, validConfig());
    TEST_ASSERT_TRUE(ctrl.isConfigured());
    const double hysteresisUpperBound = ControllerUT::getHysteresisUpperBound(ctrl);
    const double hysteresisLowerBound = ControllerUT::getHysteresisUpperBound(ctrl);
    const Pump& productPump = ControllerUT::getProductPump(ctrl);

    // Below lower bound rising
    TEST_ASSERT_EQUAL(PBRet::SUCCESS, ControllerUT::handleProductPump(ctrl, 0.0));
    TEST_ASSERT_EQUAL(Pump::PUMP_MIN_OUTPUT, productPump.getPumpSpeed());

    // Above lower bound rising
    TEST_ASSERT_EQUAL(PBRet::SUCCESS, ControllerUT::handleProductPump(ctrl, hysteresisLowerBound + 1));
    TEST_ASSERT_EQUAL(Pump::PUMP_MIN_OUTPUT, productPump.getPumpSpeed());

    // Above upper bound rising
    TEST_ASSERT_EQUAL(PBRet::SUCCESS, ControllerUT::handleProductPump(ctrl, hysteresisUpperBound + 1));
    TEST_ASSERT_EQUAL(Pump::PUMP_MAX_OUTPUT, productPump.getPumpSpeed());

    // Above lower bound falling
    TEST_ASSERT_EQUAL(PBRet::SUCCESS, ControllerUT::handleProductPump(ctrl, hysteresisLowerBound + 1));
    TEST_ASSERT_EQUAL(Pump::PUMP_MAX_OUTPUT, productPump.getPumpSpeed());

    // Below lower bound falling
    TEST_ASSERT_EQUAL(PBRet::SUCCESS, ControllerUT::handleProductPump(ctrl, 0.0));
    TEST_ASSERT_EQUAL(Pump::PUMP_MIN_OUTPUT, productPump.getPumpSpeed());
}

#ifdef __cplusplus
}
#endif
