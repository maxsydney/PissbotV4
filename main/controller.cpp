#ifdef __cplusplus
extern "C" {
#endif

#include <esp_log.h>
#include "controller.h"
#include "pump.h"
#include "gpio.h"
#include <string.h>

static char tag[] = "Controller";

Controller::Controller(uint8_t freq, Data settings, gpio_num_t P1_pin, ledc_channel_t P1_channel, 
                       ledc_timer_t timerChannel1, gpio_num_t P2_pin, ledc_channel_t P2_channel, 
                       ledc_timer_t timerChannel2, gpio_num_t fanPin, gpio_num_t elem24Pin, 
                       gpio_num_t elem3Pin):
            _updateFreq(freq), _settings(settings), _fanPin(fanPin),
            _elem24Pin(elem24Pin), _elem3Pin(elem3Pin)
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
    _refluxPump.setMode(pumpCtrl_active);
    _prodPump.setMode(pumpCtrl_active);
    _refluxPump.setSpeed(PUMP_MIN_OUTPUT);
    _prodPump.setSpeed(PUMP_MIN_OUTPUT);
}

void Controller::updatePumpSpeed(double temp)
{
    uint16_t pumpSpeed = _refluxPump.getSpeed();
    double err = temp - _settings.setpoint;
    double d_error = (err - _prevError) / _updatePeriod;
    _prevError = err;

    // Basic anti integral windup strategy
    if (pumpSpeed > PUMP_MAX_OUTPUT) {
        _integral = (PUMP_MAX_OUTPUT - _settings.P_gain * err - _settings.D_gain * d_error) / _settings.I_gain;
    } else if (pumpSpeed < PUMP_MIN_OUTPUT) {
        _integral = (PUMP_MIN_OUTPUT - _settings.P_gain * err - _settings.D_gain * d_error) / _settings.I_gain;
    } else {
        _integral += err * _updatePeriod;
    }

    double output = _settings.P_gain * err + _settings.D_gain * d_error + _settings.I_gain * _integral;

    // ESP_LOGI(tag, "P: %lf - I: %lf - D: %lf - Output: %lf", err, _integral, d_error, output);
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
    setPin(_fanPin, _fanState);;
    setPin(_elem24Pin, _elementState_24);
}

void Controller::processCommand(Cmd_t cmd)
{
    if (strncmp(cmd.cmd, "fanState", 128) == 0) {
        _fanState = atof(cmd.arg);
        setFanState(_fanState);
    } else if (strncmp(cmd.cmd, "flush", 128) == 0) {
        _flush = atof(cmd.arg);
        if (_flush) {
            ESP_LOGI(tag, "Setting both pumps to flush");
            setRefluxSpeed(FLUSH_SPEED);
            setProductSpeed(FLUSH_SPEED);
            setRefluxPumpMode(pumpCtrl_fixed);
            setProductPumpMode(pumpCtrl_fixed);
        } else {
            ESP_LOGI(tag, "Setting both pumps to active");
            setRefluxPumpMode(pumpCtrl_active);
            setProductPumpMode(pumpCtrl_active);
            setProductSpeed(PUMP_MIN_OUTPUT);
        }
    } else if (strncmp(cmd.cmd, "element1", 128) == 0)  {
        bool state = atof(cmd.arg);
        setElem24State(state);
    } else if (strncmp(cmd.cmd, "element2", 128) == 0) {
        bool state = atof(cmd.arg);
        setElem3State(state);
    } else if (strncmp(cmd.cmd, "prod", 128) == 0) {
        _prodManual = atof(cmd.arg);
        if (_prodManual) {
            ESP_LOGI(tag, "Setting prod pump to flush");
            setProductSpeed(FLUSH_SPEED);
            setProductPumpMode(pumpCtrl_fixed);
        } else {
            ESP_LOGI(tag, "Setting prod pump to active");
            setProductPumpMode(pumpCtrl_active);
            setProductSpeed(PUMP_MIN_OUTPUT);
        }
    } else {
        ESP_LOGE(tag, "Unrecognised command");
    }

    updateComponents();
}

#ifdef __cplusplus
}
#endif
