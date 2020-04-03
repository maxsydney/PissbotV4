#include <driver/gpio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "gpio.h"
#include "sensors.h"
#include "main.h"
#include "pinDefs.h"
#include "input.h"
#include "esp_log.h"

#define GPIO_PIN_BITMASK(pin) (1Ull << pin)
#define ESP_INTR_FLAG_DEFAULT 0

static const char* tag = "GPIO";

void gpio_init(void)
{
    esp_err_t err;
    gpio_config_t io_conf;

    // Set LED pin
    PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[LED_PIN], PIN_FUNC_GPIO);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_PIN, 0);

    // Set up flowmeter pin for interrupt
    io_conf.intr_type = GPIO_PIN_INTR_POSEDGE;
    io_conf.pin_bit_mask = GPIO_PIN_BITMASK(REFLUX_FLOW);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config(&io_conf);

    // Set input up button pin
    io_conf.pin_bit_mask = GPIO_PIN_BITMASK(INPUT_UP);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);  

    // Set input down button pin
    io_conf.pin_bit_mask = GPIO_PIN_BITMASK(INPUT_DOWN);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf); 

    // Set input left button pin
    io_conf.pin_bit_mask = GPIO_PIN_BITMASK(INPUT_LEFT);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf); 

    // Set input mid button pin
    io_conf.pin_bit_mask = GPIO_PIN_BITMASK(INPUT_MID);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);

    // Install interrupt services
    err = gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    if (err != ESP_OK) {
        ESP_LOGW(tag, "Something went wrong installing interrupts");
    }

    gpio_isr_handler_add(REFLUX_FLOW, flowmeter_ISR, NULL);

    // Set up fan control pin
    gpio_pad_select_gpio(FAN_SWITCH);
    gpio_set_direction(FAN_SWITCH, GPIO_MODE_OUTPUT);
    gpio_set_level(FAN_SWITCH, 0);    

    // Set up 2.4kW element control pin
    // PIN_FUNC_SELECT( IO_MUX_GPIO13_REG, PIN_FUNC_GPIO);
    gpio_pad_select_gpio(ELEMENT_2);
    gpio_set_direction(ELEMENT_2, GPIO_MODE_OUTPUT);
    gpio_set_level(ELEMENT_2, 0);    
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