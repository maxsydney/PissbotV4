#include <driver/ledc.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define PUMP_PIN 21

void pwm_init()
{
    ledc_timer_config_t ledc_timer = {
        .duty_resolution = LEDC_TIMER_13_BIT, // resolution of PWM duty
        .freq_hz = 5000,                      // frequency of PWM signal
        .speed_mode = LEDC_HIGH_SPEED_MODE,   // timer mode
        .timer_num = LEDC_TIMER_0             // timer index
    };
    ledc_timer_config(&ledc_timer);

    ledc_channel_config_t ledc_channel = {
        .channel    = LEDC_CHANNEL_0,
        .duty       = 0,
        .gpio_num   = PUMP_PIN,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .timer_sel  = LEDC_TIMER_0
    };
    ledc_channel_config(&ledc_channel);
}

void task_servoSweep(void *ignore) 
{
    uint16_t duty = 0;
	while(1) {
        duty += 10;
        ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, duty);
        ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);
        vTaskDelay(50/portTICK_PERIOD_MS);
        printf("Output: %d\n", duty);
        if (duty > 8000) {
            duty = 0;
        }
        // ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, 8000);
        // ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);
        // vTaskDelay(1000/portTICK_PERIOD_MS);
	}
}

void set_motor_speed(int speed) 
{
    ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, speed);
	ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);
}