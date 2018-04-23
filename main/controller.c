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

#define SENSOR_MIN_OUTPUT 1250
#define SENSOR_MAX_OUTPUT 8190

static Data controllerSettings = {
    .setpoint = 50,
    .P_gain = 45,
    .I_gain = 10,
    .D_gain = 300 
};

esp_err_t controller_init(uint8_t frequency)
{
    dataQueue = xQueueCreate(2, sizeof(Data));
    ctrl_loop_period_ms = 1.0 / frequency * 1000;
    int32_t setpoint, P_gain, I_gain, D_gain;

    nvs_handle nvs;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &nvs);
    if (err != ESP_OK) {
        ESP_LOGI(tag, "Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    } else {
        ESP_LOGI(tag, "NVS handle opened successfully\n");
    }

    err = nvs_get_i32(nvs, "setpoint", &setpoint);
    switch (err) {
        case ESP_OK:
            controllerSettings.setpoint = (float) setpoint / 1000;
            break;
        case ESP_ERR_NVS_NOT_FOUND:
            nvs_set_i32(nvs, "setpoint", (int32_t)(controllerSettings.setpoint * 1000));
            break;
        default:
            ESP_LOGW(tag, "Error (%s) reading!\n", esp_err_to_name(err));
    }

    err = nvs_get_i32(nvs, "P_gain", &P_gain);
    switch (err) {
        case ESP_OK:
            controllerSettings.P_gain = (float) P_gain / 1000;
            break;
        case ESP_ERR_NVS_NOT_FOUND:
            nvs_set_i32(nvs, "P_gain", (int32_t)(controllerSettings.P_gain * 1000));
            break;
        default:
            ESP_LOGW(tag, "Error (%s) reading!\n", esp_err_to_name(err));
    }

    err = nvs_get_i32(nvs, "I_gain", &I_gain);
    switch (err) {
        case ESP_OK:
            controllerSettings.I_gain = (float) I_gain / 1000;
            break;
        case ESP_ERR_NVS_NOT_FOUND:
            nvs_set_i32(nvs, "I_gain", (int32_t)(controllerSettings.I_gain * 1000));
            break;
        default:
            ESP_LOGW(tag, "Error (%s) reading!\n", esp_err_to_name(err));
    }

    err = nvs_get_i32(nvs, "D_gain", &D_gain);
    switch (err) {
        case ESP_OK:
            controllerSettings.D_gain = (float) D_gain / 1000;
            break;
        case ESP_ERR_NVS_NOT_FOUND:
            nvs_set_i32(nvs, "D_gain", (int32_t)(controllerSettings.D_gain * 1000));
            break;
        default:
            ESP_LOGW(tag, "Error (%s) reading!\n", esp_err_to_name(err));
    }

    err = nvs_commit(nvs);
    if (err != ESP_OK) {
        ESP_LOGW(tag, "Error (%s) reading!\n", esp_err_to_name(err));
    }
    nvs_close(nvs);
    return ESP_OK;
}

void control_loop(void* params)
{
    static float output;
    float temp, error, derivative;
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
        derivative = (error - last_error) / 0.2;
        last_error = error;

        // Basic strategy to avoid integral windup. Only integrate when output is not saturated
        if ((output > SENSOR_MAX_OUTPUT - 1) && (error > 0)) {
            integral += 0;
        } else if ((output < SENSOR_MIN_OUTPUT + 1) && (error < 0)) {
            integral += 0;
        } else {
            integral += error * 0.2;                                      // Begin with 1hz control loop frequency for simplicity
        }
                        
        output = controllerSettings.P_gain * error + controllerSettings.D_gain * derivative + controllerSettings.I_gain * integral;

        // ESP_LOGI(tag, "Integral error: %f", integral);
        // ESP_LOGI(tag, "Integral error: %.2f\n", derivative);

        if (output < SENSOR_MIN_OUTPUT) {
            output = SENSOR_MIN_OUTPUT;      // Ensures water is always flowing so sensor can get a reading
        } else if (output > SENSOR_MAX_OUTPUT) {
            output = SENSOR_MAX_OUTPUT;
        }
        
        // ESP_LOGI(tag, "Total output =  %.2f\n", output);
        // ESP_LOGI(tag, "Output =  %.2f + %.2f + %.2f\n", controllerSettings.P_gain * error, controllerSettings.D_gain * derivative, controllerSettings.I_gain * integral);
        set_motor_speed(output);
        vTaskDelayUntil(&xLastWakeTime, 200 / portTICK_PERIOD_MS);
    }
}

float get_temp(void)
{
    static float temp;
    float new_temp;
    if (xQueueReceive(tempQueue, &new_temp, 50 / portTICK_PERIOD_MS)) {
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

Data get_controller_settings(void)
{
    return controllerSettings;
}