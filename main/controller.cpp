#ifdef __cplusplus
extern "C" {
#endif

#include <esp_log.h>
#include "controller.h"
#include "pump.h"
#include "gpio.h"
#include <string.h>

static char tag[] = "Controller";

Controller::Controller(uint8_t freq, ctrlParams_t params, ctrlSettings_t settings, gpio_num_t P1_pin, 
                       ledc_channel_t P1_channel, ledc_timer_t timerChannel1, gpio_num_t P2_pin, 
                       ledc_channel_t P2_channel, ledc_timer_t timerChannel2, gpio_num_t fanPin, 
                       gpio_num_t elem24Pin, gpio_num_t elem3Pin):
            _updateFreq(freq), _ctrlParams(params), _ctrlSettings(settings), 
            _fanPin(fanPin), _elem24Pin(elem24Pin), _elem3Pin(elem3Pin)
{
    _updatePeriod = 1.0 / _updateFreq;
    _initPumps(P1_pin, P1_channel, timerChannel1, P2_pin, P2_channel, timerChannel2);
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

void Controller::_initPumps(gpio_num_t P1_pin, ledc_channel_t P1_channel, ledc_timer_t timerChannel1, 
                            gpio_num_t P2_pin, ledc_channel_t P2_channel, ledc_timer_t timerChannel2)
{
    _refluxPump = Pump(P1_pin, P1_channel, timerChannel1);
    _prodPump = Pump(P2_pin, P2_channel, timerChannel2);
    _refluxPump.setMode(PumpMode::ACTIVE);
    _prodPump.setMode(PumpMode::ACTIVE);
    _refluxPump.setSpeed(PUMP_MIN_OUTPUT);
    _prodPump.setSpeed(PUMP_MIN_OUTPUT);
}

void Controller::updatePumpSpeed(double temp)
{
    uint16_t pumpSpeed = _refluxPump.getSpeed();
    double err = temp - _ctrlParams.setpoint;
    double d_error = (err - _prevError) / _updatePeriod;
    _prevError = err;

    // Basic anti integral windup strategy
    // FIXME: Implement more sophisticated anti integral windup algorithm
    if ((pumpSpeed > PUMP_MAX_OUTPUT) && (err > 0)) {
        _integral += 0;
    } else if ((pumpSpeed < PUMP_MIN_OUTPUT) && (err < 0)) {
        _integral += 0;
    } else {
        _integral += err * _updatePeriod;
    }

    double output = _ctrlParams.P_gain * err + _ctrlParams.D_gain * d_error + _ctrlParams.I_gain * _integral;

    _handleProductPump(temp);
    _refluxPump.setSpeed(output);
    _refluxPump.commandPump();
    _prodPump.commandPump();
}

void Controller::_handleProductPump(double temp)
{
    if (temp > 60) {
        _prodPump.setSpeed(FLUSH_SPEED);
    } else if (temp < 50) {
        _prodPump.setSpeed(PUMP_MIN_OUTPUT);
    }
}

void Controller::updateComponents()
{
    setPin(_fanPin, _ctrlSettings.fanState);
    setPin(_elem24Pin, _ctrlSettings.elementLow);
    setPin(_elem3Pin, _ctrlSettings.elementLow);        // Can only drive one pin right now..
}

void Controller::setControllerSettings(ctrlSettings_t ctrlSettings)
{
    _ctrlSettings = ctrlSettings;
    
    if (ctrlSettings.flush == true) {
        ESP_LOGI(tag, "Setting both pumps to flush");
        setRefluxSpeed(FLUSH_SPEED);
        setProductSpeed(FLUSH_SPEED);
        setRefluxPumpMode(PumpMode::FIXED);
        setProductPumpMode(PumpMode::FIXED);
    } else {
        ESP_LOGI(tag, "Setting both pumps to active");
        setRefluxPumpMode(PumpMode::ACTIVE);
        setProductPumpMode(PumpMode::ACTIVE);
    }

    if (ctrlSettings.prodCondensor == true) {
        ESP_LOGI(tag, "Setting prod pump to flush");
        setProductSpeed(FLUSH_SPEED);
        setProductPumpMode(PumpMode::FIXED);
    } else if (_ctrlSettings.flush == false) {
        ESP_LOGI(tag, "Setting prod pump to active");
        setProductPumpMode(PumpMode::ACTIVE);
        setProductSpeed(PUMP_MIN_OUTPUT);
    }

    ESP_LOGI(tag, "Settings updated");
    ESP_LOGI(tag, "Fan: %d | 2.4kW Element: %d | 3kW Element: %d | Flush: %d | Product Condensor: %d", ctrlSettings.fanState,
             ctrlSettings.elementLow, ctrlSettings.elementHigh, ctrlSettings.flush, ctrlSettings.prodCondensor); 

    updateComponents();
}

#ifdef __cplusplus
}
#endif
