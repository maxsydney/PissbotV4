#include <driver/ledc.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "pinDefs.h"

void initPumps()
{
    // Configure timer for PWM drivers
    ledc_timer_config_t ledc_timer = {
        .duty_resolution = LEDC_TIMER_13_BIT,
        .freq_hz = 5000,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .timer_num = LEDC_TIMER_0
    };

    // Configure pump channels
    ledc_channel_config_t reflux_pwm = {
        .channel    = LEDC_CHANNEL_0,
        .duty       = 0,
        .gpio_num   = REFLUX_PUMP,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .timer_sel  = LEDC_TIMER_0
    };

    // Product pump needs PWM for soft start
    ledc_channel_config_t product_pwm = {
        .channel    = LEDC_CHANNEL_1,
        .duty       = 0,
        .gpio_num   = PROD_PUMP,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .timer_sel  = LEDC_TIMER_0
    };

    ledc_timer_config(&ledc_timer);
    ledc_channel_config(&reflux_pwm);
    ledc_channel_config(&product_pwm);
}

void set_motor_speed(int speed) 
{
    ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, speed);
	ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);
}