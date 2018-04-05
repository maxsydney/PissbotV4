#include <stdio.h>
#include <esp_log.h>
#include <esp_event.h>
#include <esp_timer.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "controller.h"
#include "pump.h"
#include "sensors.h"
#include "networking.h"
#include "main.h"
#include "gpio.h"

static char tag[] = "Controller";
static bool element_status;
static uint8_t ctrl_loop_period_ms;

static Data controllerSettings = {
    .setpoint = 28,
    .P_gain = 10,
    .I_gain = 1,
    .D_gain = 5 
};

esp_err_t controller_init(uint8_t frequency)
{
    dataQueue = xQueueCreate(2, sizeof(Data));
    ctrl_loop_period_ms = 1.0 / frequency * 1000;
    return ESP_OK;
}

void control_loop(void* params)
{
    float temp, error, derivative, output;
    float last_error = 0;
    float integral = 0;
    portTickType xLastWakeTime = xTaskGetTickCount();

    while(1) {
        if (uxQueueMessagesWaiting(dataQueue)) {
            xQueueReceive(dataQueue, &controllerSettings, 50 / portTICK_PERIOD_MS);
            flash_pin(LED_PIN, 100);
            ESP_LOGI(tag, "%s\n", "Received data from queue");
        }
        temp = get_temp();
        error =  temp - controllerSettings.setpoint;                // Order reversed because higher output reduces temperature
        derivative = error - last_error;
        integral += error;                                          // Begin with 1hz control loop frequency for simplicity
        output = controllerSettings.P_gain * error + controllerSettings.D_gain * derivative + controllerSettings.I_gain * integral;

        if (output < 0) {
            output = 0;
        } else if (output > 1024) {
            output = 1024;
        }

        printf("Controller error: %.2f\n", error);
        printf("Controller output: %.2f\n", output);

        vTaskDelayUntil(&xLastWakeTime, 1000 / portTICK_PERIOD_MS);
    }
}

float get_temp(void)
{
    static float temp;
    float new_temp;
    if (xQueueReceive(tempQueue, &new_temp, 750 / portTICK_PERIOD_MS)) {
        if (new_temp < 100) {
        temp = new_temp;        // Remove
        }
    }

    return temp;
}

float get_setpoint(void)
{
    return controllerSettings.setpoint;
}

bool get_element_status(void)
{
    return element_status;
}