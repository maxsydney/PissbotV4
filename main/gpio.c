#include <driver/gpio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "gpio.h"
#include "main.h"

void gpio_init(void)
{
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_PIN, 0);
}

void flash_pin(gpio_num_t pin, uint16_t delay)
{
    gpio_set_level(pin, GPIO_HIGH);
    vTaskDelay(delay / portTICK_PERIOD_MS);
    gpio_set_level(pin, GPIO_LOW);
}