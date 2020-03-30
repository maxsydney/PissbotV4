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
    _initPump();
}

void Pump::_initPump() const
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

    ledc_timer_config(&PWM_timer);
    ledc_channel_config(&channelConfig);

    ESP_LOGI(tag, "Pump configured on channel %d", _PWMChannel);
}

void Pump::commandPump()
{
    int16_t speed = _pumpSpeed;
    
    // Avoid over commanding pump
    if (speed < PUMP_MIN_OUTPUT) {
        speed = PUMP_MIN_OUTPUT;
    } else if (speed > PUMP_MAX_OUTPUT) {
        speed = PUMP_MAX_OUTPUT;
    }

    ledc_set_duty(LEDC_HIGH_SPEED_MODE, _PWMChannel, speed);
	ledc_update_duty(LEDC_HIGH_SPEED_MODE, _PWMChannel);
}

void Pump::setSpeed(int16_t speed)
{
    if (speed < 0) {
        speed = 0;
    }
    if (_mode == pumpCtrl_active) {
        _pumpSpeed = speed;
    } 
}

#ifdef __cplusplus
}
#endif