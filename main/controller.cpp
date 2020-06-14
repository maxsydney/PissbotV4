#ifdef __cplusplus
extern "C" {
#endif

#include <esp_log.h>
#include "controller.h"
#include "pump.h"
#include "gpio.h"
#include <string.h>

static char tag[] = "Controller";

Controller::Controller(uint8_t freq, ctrlParams_t params, ctrlSettings_t settings, const PumpCfg& refluxPumpCfg, 
                       const PumpCfg& prodPumpCfg, gpio_num_t fanPin, gpio_num_t elem24Pin, gpio_num_t elem3Pin):
            _updateFreq(freq), _ctrlParams(params), _ctrlSettings(settings), 
            _fanPin(fanPin), _elem24Pin(elem24Pin), _elem3Pin(elem3Pin)
{
    _updatePeriod = 1.0 / _updateFreq;
    _initPumps(refluxPumpCfg, prodPumpCfg);
    _initComponents();
    _prevError = 0;
    _integral = 0;
}

void Controller::_initComponents() const
{
    // Initialize fan pin
    gpio_pad_select_gpio(_fanPin);
    gpio_set_direction(_fanPin, GPIO_MODE_OUTPUT);
    gpio_set_level(_fanPin, 0);    

    // Initialize 2.4kW element control pin
    gpio_pad_select_gpio(_elem24Pin);
    gpio_set_direction(_elem24Pin, GPIO_MODE_OUTPUT);
    gpio_set_level(_elem24Pin, 0);    

    // Initialize 3kW element control pin
    gpio_pad_select_gpio(_elem3Pin);
    gpio_set_direction(_elem3Pin, GPIO_MODE_OUTPUT);
    gpio_set_level(_elem3Pin, 0);  
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
    uint16_t pumpSpeed = _refluxPump.getSpeed();
    double err = temp - _ctrlParams.setpoint;
    double d_error = (err - _prevError) / _updatePeriod;
    _prevError = err;

    // Basic anti integral windup strategy
    // FIXME: Implement more sophisticated anti integral windup algorithm
    if ((pumpSpeed >= Pump::PUMP_MAX_OUTPUT) && (err > 0)) {
        _integral += 0;
    } else if ((pumpSpeed <= Pump::PUMP_MIN_OUTPUT) && (err < 0)) {
        _integral += 0;
    } else {
        _integral += err * _updatePeriod;
    }

    double output = _ctrlParams.P_gain * err + _ctrlParams.D_gain * d_error + _ctrlParams.I_gain * _integral;

    ESP_LOGE(tag, "P term: %.3f - I term: %.3f - D term: %.3f - Total output: %.3f", _ctrlParams.P_gain * err, _ctrlParams.I_gain * _integral, _ctrlParams.D_gain * d_error, output);

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
    setPin(_fanPin, _ctrlSettings.fanState);
    setPin(_elem24Pin, _ctrlSettings.elementLow);
    setPin(_elem3Pin, _ctrlSettings.elementLow);
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

#ifdef __cplusplus
}
#endif
