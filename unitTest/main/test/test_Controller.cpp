#include "unity.h"
#include <stdio.h>
#include "main/controller.h"
#include "main/pump.h"
#include "main/pinDefs.h"

TEST_CASE("Constructor", "[Controller]")
{
    const uint8_t freq = 5;
    const ctrlParams_t ctrlParams = {50.0, 10.0, 1.0, 1.0};
    const ctrlSettings_t ctrlSettings = {};
    PumpCfg refluxPumpCfg(REFLUX_PUMP, LEDC_CHANNEL_0, LEDC_TIMER_0);
    PumpCfg prodPumpCfg(PROD_PUMP, LEDC_CHANNEL_1, LEDC_TIMER_1);
    Controller Ctrl = Controller(freq, ctrlParams, ctrlSettings, refluxPumpCfg,  prodPumpCfg, FAN_SWITCH, ELEMENT_2, ELEMENT_2);

    TEST_ASSERT_EQUAL_DOUBLE(Ctrl.getSetpoint(), 50.0);
    TEST_ASSERT_EQUAL_DOUBLE(Ctrl.getPGain(), 10.0);
    TEST_ASSERT_EQUAL_DOUBLE(Ctrl.getIGain(), 1.0);
    TEST_ASSERT_EQUAL_DOUBLE(Ctrl.getDGain(), 1.0);

    TEST_ASSERT_TRUE(Ctrl.getRefluxPumpMode() == PumpMode::ACTIVE);
    TEST_ASSERT_TRUE(Ctrl.getProductPumpMode() == PumpMode::ACTIVE);
    TEST_ASSERT_EQUAL_UINT16(Ctrl.getRefluxPumpSpeed(), Pump::PUMP_MIN_OUTPUT);
    TEST_ASSERT_EQUAL_UINT16(Ctrl.getProductPumpSpeed(), Pump::PUMP_MIN_OUTPUT);
}

TEST_CASE("Proportional Control Response", "[Controller]")
{
    // Setup proportional only control
    const uint8_t freq = 5;
    const ctrlParams_t ctrlParams = {0.0, 1.0, 0.0, 0.0};
    const ctrlSettings_t ctrlSettings = {};
    PumpCfg refluxPumpCfg(REFLUX_PUMP, LEDC_CHANNEL_0, LEDC_TIMER_0);
    PumpCfg prodPumpCfg(PROD_PUMP, LEDC_CHANNEL_1, LEDC_TIMER_1);
    Controller Ctrl = Controller(freq, ctrlParams, ctrlSettings, refluxPumpCfg,  prodPumpCfg, FAN_SWITCH, ELEMENT_2, ELEMENT_2);

    Ctrl.updatePumpSpeed(25.0);
    TEST_ASSERT_EQUAL_INT16(25, Ctrl.getRefluxPumpSpeed());

    Ctrl.updatePumpSpeed(50.0);
    TEST_ASSERT_EQUAL_INT16(50, Ctrl.getRefluxPumpSpeed());

    Ctrl.updatePumpSpeed(75.0);
    TEST_ASSERT_EQUAL_INT16(75, Ctrl.getRefluxPumpSpeed());
}

TEST_CASE("Integral Control Response", "[Controller]")
{
    // Setup integral only control
    const uint8_t freq = 1;
    const ctrlParams_t ctrlParams = {0.0, 0.0, 1.0, 0.0};
    const ctrlSettings_t ctrlSettings = {};
    PumpCfg refluxPumpCfg(REFLUX_PUMP, LEDC_CHANNEL_0, LEDC_TIMER_0);
    PumpCfg prodPumpCfg(PROD_PUMP, LEDC_CHANNEL_1, LEDC_TIMER_1);
    Controller Ctrl = Controller(freq, ctrlParams, ctrlSettings, refluxPumpCfg,  prodPumpCfg, FAN_SWITCH, ELEMENT_2, ELEMENT_2);

    Ctrl.updatePumpSpeed(25.0);
    TEST_ASSERT_EQUAL_INT16(25, Ctrl.getRefluxPumpSpeed());

    Ctrl.updatePumpSpeed(25.0);
    TEST_ASSERT_EQUAL_INT16(50, Ctrl.getRefluxPumpSpeed());

    Ctrl.updatePumpSpeed(25.0);
    TEST_ASSERT_EQUAL_INT16(75, Ctrl.getRefluxPumpSpeed());
}

TEST_CASE("Derivative Control Response", "[Controller]")
{
    // Setup derivative only control
    const uint8_t freq = 1;
    const ctrlParams_t ctrlParams = {0.0, 0.0, 0.0, 1.0};
    const ctrlSettings_t ctrlSettings = {};
    PumpCfg refluxPumpCfg(REFLUX_PUMP, LEDC_CHANNEL_0, LEDC_TIMER_0);
    PumpCfg prodPumpCfg(PROD_PUMP, LEDC_CHANNEL_1, LEDC_TIMER_1);
    Controller Ctrl = Controller(freq, ctrlParams, ctrlSettings, refluxPumpCfg,  prodPumpCfg, FAN_SWITCH, ELEMENT_2, ELEMENT_2);

    Ctrl.updatePumpSpeed(50.0);
    TEST_ASSERT_EQUAL_INT16(50, Ctrl.getRefluxPumpSpeed());

    Ctrl.updatePumpSpeed(75.0);
    TEST_ASSERT_EQUAL_INT16(25, Ctrl.getRefluxPumpSpeed());

    Ctrl.updatePumpSpeed(175);
    TEST_ASSERT_EQUAL_INT16(100, Ctrl.getRefluxPumpSpeed());
}

TEST_CASE("PID control Response", "[Controller]")
{
    // Setup full PID control
    const uint8_t freq = 1;
    const ctrlParams_t ctrlParams = {0.0, 1.0, 1.0, 1.0};
    const ctrlSettings_t ctrlSettings = {};
    PumpCfg refluxPumpCfg(REFLUX_PUMP, LEDC_CHANNEL_0, LEDC_TIMER_0);
    PumpCfg prodPumpCfg(PROD_PUMP, LEDC_CHANNEL_1, LEDC_TIMER_1);
    Controller Ctrl = Controller(freq, ctrlParams, ctrlSettings, refluxPumpCfg,  prodPumpCfg, FAN_SWITCH, ELEMENT_2, ELEMENT_2);

    // Proportional is 25, integral is 25, derivative is 25
    Ctrl.updatePumpSpeed(25.0);
    TEST_ASSERT_EQUAL_INT16(75, Ctrl.getRefluxPumpSpeed());

    // Proportional is 25, integral is 50, derivative is 0
    Ctrl.updatePumpSpeed(25.0);
    TEST_ASSERT_EQUAL_INT16(75, Ctrl.getRefluxPumpSpeed());

    // Proportional is 50, integral is 100, derivative is 25
    Ctrl.updatePumpSpeed(50);
    TEST_ASSERT_EQUAL_INT16(175, Ctrl.getRefluxPumpSpeed());

    // Proportional is 50, integral is 150, derivative is 0
    Ctrl.updatePumpSpeed(50);
    TEST_ASSERT_EQUAL_INT16(200, Ctrl.getRefluxPumpSpeed());

    // Proportional is 100, integral is 250, derivative is 50
    Ctrl.updatePumpSpeed(100);
    TEST_ASSERT_EQUAL_INT16(400, Ctrl.getRefluxPumpSpeed());

    // Proportional is 50, integral is 300, derivative is -50
    Ctrl.updatePumpSpeed(50);
    TEST_ASSERT_EQUAL_INT16(300, Ctrl.getRefluxPumpSpeed());

    // Proportional is -50, integral is 250, derivative is -100
    Ctrl.updatePumpSpeed(-50);
    TEST_ASSERT_EQUAL_INT16(100, Ctrl.getRefluxPumpSpeed());

    // Proportional is -25, integral is 225, derivative is 25
    Ctrl.updatePumpSpeed(-25);
    TEST_ASSERT_EQUAL_INT16(225, Ctrl.getRefluxPumpSpeed());

    // Proportional is -25, integral is 200, derivative is 0
    Ctrl.updatePumpSpeed(-25);
    TEST_ASSERT_EQUAL_INT16(175, Ctrl.getRefluxPumpSpeed());

    // Proportional is -25, integral is 175, derivative is 0
    Ctrl.updatePumpSpeed(-25);
    TEST_ASSERT_EQUAL_INT16(150, Ctrl.getRefluxPumpSpeed());
}

TEST_CASE("Output Saturation", "[Controller]")
{
    // Setup proportional only control
    const uint8_t freq = 1;
    const ctrlParams_t ctrlParams = {0.0, 1.0, 0.0, 0.0};
    const ctrlSettings_t ctrlSettings = {};
    PumpCfg refluxPumpCfg(REFLUX_PUMP, LEDC_CHANNEL_0, LEDC_TIMER_0);
    PumpCfg prodPumpCfg(PROD_PUMP, LEDC_CHANNEL_1, LEDC_TIMER_1);
    Controller Ctrl = Controller(freq, ctrlParams, ctrlSettings, refluxPumpCfg,  prodPumpCfg, FAN_SWITCH, ELEMENT_2, ELEMENT_2);

    // Check minimum speed
    Ctrl.updatePumpSpeed(0);
    TEST_ASSERT_EQUAL_INT16(Pump::PUMP_MIN_OUTPUT, Ctrl.getRefluxPumpSpeed());
    
    // Check maximum speed
    Ctrl.updatePumpSpeed(10000);
    TEST_ASSERT_EQUAL_INT16(Pump::PUMP_MAX_OUTPUT, Ctrl.getRefluxPumpSpeed());

    // Check normal speed
    Ctrl.updatePumpSpeed(75.0);
    TEST_ASSERT_EQUAL_INT16(75, Ctrl.getRefluxPumpSpeed());
}

TEST_CASE("Auto Product Condensor", "[Controller]")
{
    const uint8_t freq = 1;
    const ctrlParams_t ctrlParams = {0.0, 1.0, 0.0, 0.0};
    const ctrlSettings_t ctrlSettings = {};
    PumpCfg refluxPumpCfg(REFLUX_PUMP, LEDC_CHANNEL_0, LEDC_TIMER_0);
    PumpCfg prodPumpCfg(PROD_PUMP, LEDC_CHANNEL_1, LEDC_TIMER_1);
    Controller Ctrl = Controller(freq, ctrlParams, ctrlSettings, refluxPumpCfg,  prodPumpCfg, FAN_SWITCH, ELEMENT_2, ELEMENT_2);

    TEST_ASSERT_EQUAL_INT16(Pump::PUMP_MIN_OUTPUT, Ctrl.getProductPumpSpeed());

    // Set temp above 60 degrees
    Ctrl.updatePumpSpeed(75);
    TEST_ASSERT_EQUAL_INT16(Pump::FLUSH_SPEED, Ctrl.getProductPumpSpeed());
}

TEST_CASE("Set Controller Settings", "[Controller]")
{
    const uint8_t freq = 1;
    const ctrlParams_t ctrlParams = {1.0, 1.0, 1.0, 1.0};
    ctrlSettings_t ctrlSettingsIn = {};
    ctrlSettings_t ctrlSettingsOut = {};
    PumpCfg refluxPumpCfg(REFLUX_PUMP, LEDC_CHANNEL_0, LEDC_TIMER_0);
    PumpCfg prodPumpCfg(PROD_PUMP, LEDC_CHANNEL_1, LEDC_TIMER_1);
    Controller Ctrl = Controller(freq, ctrlParams, ctrlSettingsIn, refluxPumpCfg,  prodPumpCfg, FAN_SWITCH, ELEMENT_2, ELEMENT_2);

    // Check all settings as defaults
    ctrlSettingsOut = Ctrl.getControllerSettings();
    TEST_ASSERT_EQUAL_INT(0, ctrlSettingsOut.fanState);
    TEST_ASSERT_EQUAL_INT(0, ctrlSettingsOut.flush);
    TEST_ASSERT_EQUAL_INT(0, ctrlSettingsOut.elementLow);
    TEST_ASSERT_EQUAL_INT(0, ctrlSettingsOut.elementHigh);
    TEST_ASSERT_EQUAL_INT(0, ctrlSettingsOut.prodCondensor);
    TEST_ASSERT_TRUE(Ctrl.getRefluxPumpMode() == PumpMode::ACTIVE);
    TEST_ASSERT_TRUE(Ctrl.getProductPumpMode() == PumpMode::ACTIVE);

    // Set fan on
    ctrlSettingsIn.fanState = 1;
    Ctrl.setControllerSettings(ctrlSettingsIn);
    ctrlSettingsOut = Ctrl.getControllerSettings();
    TEST_ASSERT_EQUAL_INT(1, ctrlSettingsOut.fanState);

    // Set flush
    ctrlSettingsIn.flush = 1;
    Ctrl.setControllerSettings(ctrlSettingsIn);
    ctrlSettingsOut = Ctrl.getControllerSettings();
    TEST_ASSERT_EQUAL_INT(1, ctrlSettingsOut.flush);
    TEST_ASSERT_EQUAL_INT16(Pump::FLUSH_SPEED, Ctrl.getRefluxPumpSpeed());
    TEST_ASSERT_EQUAL_INT16(Pump::FLUSH_SPEED, Ctrl.getProductPumpSpeed());
    TEST_ASSERT_TRUE(Ctrl.getRefluxPumpMode() == PumpMode::FIXED);
    TEST_ASSERT_TRUE(Ctrl.getProductPumpMode() == PumpMode::FIXED);

    // Set elementLow
    ctrlSettingsIn.elementLow = 1;
    Ctrl.setControllerSettings(ctrlSettingsIn);
    ctrlSettingsOut = Ctrl.getControllerSettings();
    TEST_ASSERT_EQUAL_INT(1, ctrlSettingsOut.elementLow);

    // Set elementHigh
    ctrlSettingsIn.elementHigh = 1;
    Ctrl.setControllerSettings(ctrlSettingsIn);
    ctrlSettingsOut = Ctrl.getControllerSettings();
    TEST_ASSERT_EQUAL_INT(1, ctrlSettingsOut.elementHigh);

    // Set productCondensor
    ctrlSettingsIn.prodCondensor = 1;
    Ctrl.setControllerSettings(ctrlSettingsIn);
    ctrlSettingsOut = Ctrl.getControllerSettings();
    TEST_ASSERT_EQUAL_INT(1, ctrlSettingsOut.prodCondensor);
    TEST_ASSERT_EQUAL_INT16(Pump::FLUSH_SPEED, Ctrl.getProductPumpSpeed());
    TEST_ASSERT_TRUE(Ctrl.getProductPumpMode() == PumpMode::FIXED);
}

