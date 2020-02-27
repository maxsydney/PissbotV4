#include <driver/gpio.h>
#include <esp_log.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "gpio.h"
#include "sensors.h"
#include "main.h"
#include "pinDefs.h"
#include "input.h"

static char tag[] = "Input";
xQueueHandle inputQueue;

esp_err_t init_input(void)
{
    inputQueue = xQueueCreate(10, sizeof(buttonPress));
    ESP_LOGI(tag, "Inputs initialized");
    return ESP_OK;
}

void inputButtonTask(void* param)
{
    // Setup data storage required
    portTickType xLastWakeTime = xTaskGetTickCount();
    uint8_t btnUpCounter = 0;
    uint8_t btnDownCounter = 0;
    uint8_t btnLeftCounter = 0;
    uint8_t btnMidCounter = 0;
    const int btnThresh = 20;

    // Run main loop and poll buttons every n ms
    while (true) {
        // Poll pins
        if (gpio_get_level(INPUT_UP)) {
            btnUpCounter++;
        } else {
            btnUpCounter = 0;
        }

        if (gpio_get_level(INPUT_DOWN)) {
            btnDownCounter++;
        } else {
            btnDownCounter = 0;
        }

        if (gpio_get_level(INPUT_LEFT)) {
            btnLeftCounter++;
        } else {
            btnLeftCounter = 0;
        }

        if (gpio_get_level(INPUT_MID)) {
            btnMidCounter++;
        } else {
            btnMidCounter = 0;
        }

        // Check counters
        if (btnUpCounter >= btnThresh) {
            buttonPress btn = {.button = input_up};
            xQueueSend(inputQueue, &btn, 10 / portTICK_PERIOD_MS);
            btnUpCounter = 0;
        }

        // Check counters
        if (btnDownCounter >= btnThresh) {
            buttonPress btn = {.button = input_down};
            xQueueSend(inputQueue, &btn, 10 / portTICK_PERIOD_MS);
            btnDownCounter = 0;
        }

        // Check counters
        if (btnLeftCounter >= btnThresh) {
            buttonPress btn = {.button = input_left};
            xQueueSend(inputQueue, &btn, 10 / portTICK_PERIOD_MS);
            btnLeftCounter = 0;
        }

        // Check counters
        if (btnMidCounter >= btnThresh) {
            buttonPress btn = {.button = input_mid};
            xQueueSend(inputQueue, &btn, 10 / portTICK_PERIOD_MS);
            btnMidCounter = 0;
        }

        vTaskDelayUntil(&xLastWakeTime, 5 / portTICK_PERIOD_MS);
    }
}