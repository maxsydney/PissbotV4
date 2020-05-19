#ifdef __cplusplus
extern "C" {
#endif

#include <driver/ledc.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <esp_log.h>
#include "pinDefs.h"
#include "pump.h"
#include "controller.h"
#include <stdint.h>

static char tag[] = "Pump";

Pump::Pump(gpio_num_t pin, ledc_channel_t PWMChannel, ledc_timer_t timerChannel):
    _PWMChannel(PWMChannel), _pin(pin), _timerChannel(timerChannel)
{
    esp_err_t err = _initPump();
    if (err == ESP_OK) {
        _configured = true;
    } else {
        _configured = false;
    }
}

esp_err_t Pump::_initPump() const
{
    // Configure timer for PWM drivers
    ledc_timer_config_t PWM_timer;
    PWM_timer.duty_resolution = LEDC_TIMER_9_BIT;
    PWM_timer.freq_hz = 5000;
    PWM_timer.speed_mode = LEDC_HIGH_SPEED_MODE;
    PWM_timer.timer_num = _timerChannel;
    PWM_timer.clk_cfg = LEDC_AUTO_CLK;

    // Configure pump channels
    ledc_channel_config_t channelConfig;
    channelConfig.channel    = _PWMChannel;
    channelConfig.duty       = 0;
    channelConfig.gpio_num   = (int) _pin;
    channelConfig.speed_mode = LEDC_HIGH_SPEED_MODE;
    channelConfig.timer_sel  = _timerChannel;
    channelConfig.hpoint = 0xff;

    esp_err_t err = ledc_timer_config(&PWM_timer);
    if (err == ESP_ERR_INVALID_ARG) {
        ESP_LOGE(tag, "Invalid parameter passed to timer configuration. Unable to configure pump on channel %d", _PWMChannel);
        return err;
    } else if (err == ESP_FAIL) {
        ESP_LOGE(tag, "Can not find a proper pre-divider number base on the given frequency and the current duty_resolution. Pump configured on channel %d", _PWMChannel);
        return err;
    }

    err = ledc_channel_config(&channelConfig);
    if (err == ESP_OK) {
        ESP_LOGI(tag, "Pump configured on channel %d", _PWMChannel);
    } else {
        ESP_LOGW(tag, "Invalid argument supplied to channel config. Unable to configure pump on channel %d", _PWMChannel);
    }
    
    return err;
}

void Pump::commandPump()
{
    ledc_set_duty(LEDC_HIGH_SPEED_MODE, _PWMChannel, _pumpSpeed);
	ledc_update_duty(LEDC_HIGH_SPEED_MODE, _PWMChannel);
}

void Pump::setSpeed(int16_t speed)
{
    // Saturate output command
    if (speed < PUMP_MIN_OUTPUT) {
        speed = PUMP_MIN_OUTPUT;
    } else if (speed > PUMP_MAX_OUTPUT) {
        speed = PUMP_MAX_OUTPUT;
    }
    
    // Only command pump if in active control mode
    if (_pumpMode == PumpMode::ACTIVE) {
        _pumpSpeed = speed;
    } 
}

#ifdef __cplusplus
}
#endif