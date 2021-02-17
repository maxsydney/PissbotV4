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
        static Pump& getProductPump(Controller& ctrl) { return ctrl._productPump; }
        static double getHysteresisUpperBound(Controller& ctrl) { return ctrl.HYSTERESIS_BOUND_UPPER; }
        static double getHysteresisLowerBound(Controller& ctrl) { return ctrl.HYSTERESIS_BOUND_LOWER; }
        static PBRet updateProductPump(Controller& ctrl, double T) { return ctrl._updateProductPump(T); }
        static PBRet initIO(Controller& ctrl, const ControllerConfig& cfg) { return ctrl._initIO(cfg); }
        static PBRet initPumps(Controller& ctrl, const PumpConfig& refluxCfg, const PumpConfig prodCfg) { return ctrl._initPumps(refluxCfg, prodCfg); }
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

TEST_CASE("updateProductPump", "[Controller]")
{
    Controller ctrl(1, 1024, 1, validConfig());
    TEST_ASSERT_TRUE(ctrl.isConfigured());
    const double hysteresisUpperBound = ControllerUT::getHysteresisUpperBound(ctrl);
    const double hysteresisLowerBound = ControllerUT::getHysteresisLowerBound(ctrl);
    const Pump& productPump = ControllerUT::getProductPump(ctrl);

    // Below lower bound rising
    TEST_ASSERT_EQUAL(PBRet::SUCCESS, ControllerUT::updateProductPump(ctrl, 0.0));
    TEST_ASSERT_EQUAL(Pump::PUMP_IDLE_SPEED, productPump.getPumpSpeed());

    // Above lower bound rising
    TEST_ASSERT_EQUAL(PBRet::SUCCESS, ControllerUT::updateProductPump(ctrl, hysteresisLowerBound + 1));
    TEST_ASSERT_EQUAL(Pump::PUMP_IDLE_SPEED, productPump.getPumpSpeed());

    // Above upper bound rising
    TEST_ASSERT_EQUAL(PBRet::SUCCESS, ControllerUT::updateProductPump(ctrl, hysteresisUpperBound + 1));
    TEST_ASSERT_EQUAL(Pump::PUMP_MAX_SPEED, productPump.getPumpSpeed());

    // Above lower bound falling
    TEST_ASSERT_EQUAL(PBRet::SUCCESS, ControllerUT::updateProductPump(ctrl, hysteresisLowerBound + 1));
    TEST_ASSERT_EQUAL(Pump::PUMP_MAX_SPEED, productPump.getPumpSpeed());

    // Below lower bound falling
    TEST_ASSERT_EQUAL(PBRet::SUCCESS, ControllerUT::updateProductPump(ctrl, 0.0));
    TEST_ASSERT_EQUAL(Pump::PUMP_IDLE_SPEED, productPump.getPumpSpeed());
}

#ifdef __cplusplus
}
#endif
