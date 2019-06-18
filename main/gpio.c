#include <driver/gpio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "gpio.h"
#include "sensors.h"
#include "main.h"
#include "pinDefs.h"
#include "input.h"

#define GPIO_PIN_BITMASK(pin) (1Ull << pin)
#define ESP_INTR_FLAG_DEFAULT 0

void gpio_init(void)
{
    esp_err_t err;
    gpio_config_t io_conf;

    // Set LED pin
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
    io_conf.intr_type = GPIO_PIN_INTR_POSEDGE;
    io_conf.pin_bit_mask = GPIO_PIN_BITMASK(INPUT_UP);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);  

<<<<<<< HEAD
    // Set input down button pin
    io_conf.intr_type = GPIO_PIN_INTR_POSEDGE;
    io_conf.pin_bit_mask = GPIO_PIN_BITMASK(INPUT_DOWN);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf); 

=======
    // Set input up button pin
    io_conf.intr_type = GPIO_PIN_INTR_POSEDGE;
    io_conf.pin_bit_mask = GPIO_PIN_BITMASK(INPUT_UP);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);  

    // Set input down button pin
    io_conf.intr_type = GPIO_PIN_INTR_POSEDGE;
    io_conf.pin_bit_mask = GPIO_PIN_BITMASK(INPUT_DOWN);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf); 

>>>>>>> 42f4ce2a03db0fc47d41de576334adf443e59735
    // Set input left button pin
    io_conf.intr_type = GPIO_PIN_INTR_POSEDGE;
    io_conf.pin_bit_mask = GPIO_PIN_BITMASK(INPUT_LEFT);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf); 

    // Set input mid button pin
    io_conf.intr_type = GPIO_PIN_INTR_POSEDGE;
    io_conf.pin_bit_mask = GPIO_PIN_BITMASK(INPUT_MID);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);

    // Install interrupt services
    err = gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    if (err != ESP_OK) {
        printf("Something went wrong installing interrupts\n");
    }

    gpio_isr_handler_add(REFLUX_FLOW, flowmeter_ISR, NULL);
    gpio_isr_handler_add(INPUT_UP, input_ISR_handler, (void*)input_up);
    gpio_isr_handler_add(INPUT_DOWN, input_ISR_handler, (void*)input_down);
    gpio_isr_handler_add(INPUT_LEFT, input_ISR_handler, (void*)input_left); 
    gpio_isr_handler_add(INPUT_MID, input_ISR_handler, (void*)input_mid);  

    // Set up fan control pin
    gpio_pad_select_gpio(FAN_SWITCH);
    gpio_set_direction(FAN_SWITCH, GPIO_MODE_OUTPUT);
    gpio_set_level(FAN_SWITCH, 0);    
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