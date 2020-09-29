#ifdef __cplusplus
extern "C" {
#endif

#include <esp_log.h>
#include "controller.h"
#include "pump.h"
#include "webServer.h"
#include "gpio.h"
#include <cmath>
#include <string.h>
#include "distillerManager.h"

// TODO: Replace this with class name
static char tag[] = "Controller";

Controller::Controller(const ControllerConfig& cfg)
{
    if (Controller::checkInputs(cfg) != PBRet::SUCCESS) {
        _configured = false;
        return;
    }
    
    _cfg = cfg;
    _initPumps(cfg.refluxPumpCfg, cfg.prodPumpCfg);
    _initComponents();
}

void Controller::_initComponents() const
{
    // Initialize fan pin
    gpio_pad_select_gpio(_cfg.fanPin);
    gpio_set_direction(_cfg.fanPin, GPIO_MODE_OUTPUT);
    gpio_set_level(_cfg.fanPin, 0);    
    
    // Initialize 2.4kW element control pin
    PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[_cfg.element1Pin], PIN_FUNC_GPIO);        // Element 1 pin is set to JTAG by default. Reassign to GPIO
    gpio_set_direction(_cfg.element1Pin, GPIO_MODE_OUTPUT);
    gpio_set_level(_cfg.element1Pin, 0);  

    // Initialize 3kW element control pin
    gpio_pad_select_gpio(_cfg.element2Pin);
    gpio_set_direction(_cfg.element2Pin, GPIO_MODE_OUTPUT);
    gpio_set_level(_cfg.element2Pin, 0); 
}

void Controller::_initPumps(const PumpCfg& refluxPumpCfg, const PumpCfg& prodPumpCfg)
{
    _refluxPump = Pump(refluxPumpCfg);
    _prodPump = Pump(prodPumpCfg);
    _refluxPump.setMode(PumpMode::ACTIVE);
    _prodPump.setMode(PumpMode::ACTIVE);
    _refluxPump.setSpeed(Pump::PUMP_MIN_OUTPUT);
    _prodPump.setSpeed(Pump::PUMP_MIN_OUTPUT);
}

void Controller::updatePumpSpeed(double temp)
{
    const double dt = 1.0 / _cfg.updateFreqHz;
    double err = temp - _ctrlParams.setpoint;

    // Proportional term
    double proportional = _ctrlParams.P_gain * err;

    // Integral term (discretized via bilinear transform)
    _integral += 0.5 * _ctrlParams.I_gain * dt * (err + _prevError);

    // Dynamic integral clamping
    double intLimMin = 0.0;
    double intLimMax = 0.0;

    if (proportional < Pump::PUMP_MAX_OUTPUT) {
        intLimMax = Pump::PUMP_MAX_OUTPUT - proportional;
    } else {
        intLimMax = 0.0;
    }

    // Anti integral windup
    if (proportional > Pump::PUMP_MIN_OUTPUT) {
        intLimMin = Pump::PUMP_MIN_OUTPUT - proportional;
    } else {
        intLimMin = 0.0;
    }

    if (_integral > intLimMax) {
        _integral = intLimMax;
    } else if (_integral < intLimMin) {
        _integral = intLimMin;
    }

    // Derivative term (discretized via backwards temperature differentiation)
    // TODO: Filtering on D term? Quite tricky due to low temp sensor sample rate
    const double alpha = 0.15;      // TODO: Improve hacky dterm filter
    _derivative = (1 - alpha) * _derivative + alpha * (_ctrlParams.D_gain * (temp - _prevTemp) / dt);

    // Compute limited output
    double output = proportional + _integral + _derivative;

    if (output > Pump::PUMP_MAX_OUTPUT) {
        output = Pump::PUMP_MAX_OUTPUT;
    } else if (output < Pump::PUMP_MIN_OUTPUT) {
        output = Pump::PUMP_MIN_OUTPUT;
    }

    // Debugging only, spams the network
    // printf("Temp: %.3f - Err: %.3f - P term: %.3f - I term: %.3f - D term: %.3f - Total output: %.3f\n", temp, err, proportional, _integral, _derivative, output);

    _prevError = err;
    _prevTemp = temp;
    _handleProductPump(temp);
    _refluxPump.setSpeed(output);
    _refluxPump.commandPump();
    _prodPump.commandPump();
}

void Controller::_handleProductPump(double temp)
{
    if (temp > 60) {
        _prodPump.setSpeed(Pump::FLUSH_SPEED);
    } else if (temp < 50) {
        _prodPump.setSpeed(Pump::PUMP_MIN_OUTPUT);
    }
}

void Controller::updateComponents()
{
    setPin(_cfg.fanPin, _ctrlSettings.fanState);
    setPin(_cfg.element1Pin, _ctrlSettings.elementLow);
    setPin(_cfg.element2Pin, _ctrlSettings.elementHigh);
}

void Controller::setControllerSettings(ctrlSettings_t ctrlSettings)
{
    _ctrlSettings = ctrlSettings;
    
    if (ctrlSettings.flush == true) {
        ESP_LOGI(tag, "Setting both pumps to flush");
        setRefluxSpeed(Pump::FLUSH_SPEED);
        setProductSpeed(Pump::FLUSH_SPEED);
        setRefluxPumpMode(PumpMode::FIXED);
        setProductPumpMode(PumpMode::FIXED);
    } else {
        ESP_LOGI(tag, "Setting both pumps to active");
        setRefluxPumpMode(PumpMode::ACTIVE);
        setProductPumpMode(PumpMode::ACTIVE);
    }

    if (ctrlSettings.prodCondensor == true) {
        ESP_LOGI(tag, "Setting prod pump to flush");
        setProductSpeed(Pump::FLUSH_SPEED);
        setProductPumpMode(PumpMode::FIXED);
    } else if (_ctrlSettings.flush == false) {
        ESP_LOGI(tag, "Setting prod pump to active");
        setProductPumpMode(PumpMode::ACTIVE);
        setProductSpeed(Pump::PUMP_MIN_OUTPUT);
    }

    ESP_LOGI(tag, "Settings updated");
    ESP_LOGI(tag, "Fan: %d | 2.4kW Element: %d | 3kW Element: %d | Flush: %d | Product Condensor: %d", ctrlSettings.fanState,
             ctrlSettings.elementLow, ctrlSettings.elementHigh, ctrlSettings.flush, ctrlSettings.prodCondensor); 

    updateComponents();
}

PBRet Controller::checkInputs(const ControllerConfig& cfg)
{
    if (cfg.updateFreqHz <= 0) {
        ESP_LOGE(tag, "Update frequency %d is invalid. Controller was not configured", cfg.updateFreqHz);
        return PBRet::FAILURE;
    }

    // TODO: Call Pump checkInputs here

    // TODO: Check if these pins are valid output pins
    if (cfg.fanPin <= 0) {
        ESP_LOGE(tag, "Fan GPIO %d is invalid. Controller was not configured", cfg.fanPin);
        return PBRet::FAILURE;
    }

    if (cfg.element1Pin <= 0) {
        ESP_LOGE(tag, "Element 1 GPIO %d is invalid. Controller was not configured", cfg.element1Pin);
        return PBRet::FAILURE;
    }
    
    if (cfg.element2Pin <= 0) {
        ESP_LOGE(tag, "Element 1 GPIO %d is invalid. Controller was not configured", cfg.element2Pin);
        return PBRet::FAILURE;
    }

    return PBRet::SUCCESS;
}

#ifdef __cplusplus
}
#endif
