#include <driver/gpio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "gpio.h"
#include "sensors.h"
#include "main.h"
#include "pinDefs.h"

#define GPIO_PIN_BITMASK(pin) (1Ull << pin)
#define ESP_INTR_FLAG_DEFAULT 0

void gpio_init(void)
{
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_PIN, 0);

    // Set up flowmeter pin for interrupt
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_PIN_INTR_POSEDGE;
    io_conf.pin_bit_mask = GPIO_PIN_BITMASK(REFLUX_FLOW);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);


    // Set up fan control pin for
    gpio_pad_select_gpio(FAN_SWITCH);
    gpio_set_direction(FAN_SWITCH, GPIO_MODE_OUTPUT);
    gpio_set_level(FAN_SWITCH, 0);

    esp_err_t err = gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    if (err != ESP_OK) {
        printf("Something went wrong\n");
    }
    gpio_isr_handler_add(REFLUX_FLOW, flowmeter_ISR, NULL);
}

void flash_pin(gpio_num_t pin, uint16_t delay)
{
    gpio_set_level(pin, GPIO_HIGH);
    vTaskDelay(delay / portTICK_PERIOD_MS);
    gpio_set_level(pin, GPIO_LOW);
}

void setPin(gpio_num_t pin, bool state)
{
    gpio_set_level(pin, state);
}