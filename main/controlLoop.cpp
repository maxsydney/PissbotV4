#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <esp_log.h>
#include <esp_event.h>
#include <esp_timer.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "controlLoop.h"
#include "pump.h"
#include "gpio.h"
#include "controller.h"
#include "sensors.h"
#include "networking.h"
#include "main.h"
#include "driver/gpio.h"
#include "messages.h"
#include "pinDefs.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Controller constants
#define SENSOR_MIN_OUTPUT 1600
#define SENSOR_MAX_OUTPUT 8190
#define FAN_THRESH 30
#define P_atm 101.325

// Antoine equation constants
#define H20_A 10.196213
#define H20_B 1730.63
#define H20_C 233.426

#define ETH_A 9.806073      // Parameters valid for T in (77, 243) degrees celsius
#define ETH_B 1332.04
#define ETH_C 199.200

static char tag[] = "Control Loop";
static ctrlParams_t controllerParams = {};
static ctrlSettings_t controllerSettings = {};
xQueueHandle ctrlParamsQueue;
xQueueHandle ctrlSettingsQueue;
uint16_t ctrl_loop_period_ms;

esp_err_t controller_init(uint8_t frequency)
{
    ctrlParamsQueue = xQueueCreate(10, sizeof(ctrlParams_t));
    ctrlSettingsQueue = xQueueCreate(10, sizeof(ctrlSettings_t));
    ctrl_loop_period_ms = 1.0 / frequency * 1000;

    ESP_LOGI(tag, "Controller initialized");

    return ESP_OK;
}

void control_loop(void* params)
{
    float temperatures[n_tempSensors] = {0};
    ctrlParams_t ctrlParams = getSettingsFromNVM();
    controllerParams = ctrlParams;
    ctrlSettings_t ctrlSettings = {};
    Controller Ctrl = Controller(CONTROL_LOOP_FREQUENCY, ctrlParams, ctrlSettings, REFLUX_PUMP, LEDC_CHANNEL_0, LEDC_TIMER_0, PROD_PUMP, LEDC_CHANNEL_1, LEDC_TIMER_1, FAN_SWITCH, ELEMENT_2, ELEMENT_2);
    portTickType xLastWakeTime = xTaskGetTickCount();
    ESP_LOGI(tag, "Control loop active");
    
    while (true) {
        if (uxQueueMessagesWaiting(ctrlParamsQueue)) {
            xQueueReceive(ctrlParamsQueue, &ctrlParams, 50 / portTICK_PERIOD_MS);
            flash_pin(LED_PIN, 100);
            controllerParams = ctrlParams;
            Ctrl.setControllerParams(ctrlParams);
            ESP_LOGI(tag, "Controller parameters updated");
        }

        if (uxQueueMessagesWaiting(ctrlSettingsQueue)) {
            xQueueReceive(ctrlSettingsQueue, &ctrlSettings, 50 / portTICK_PERIOD_MS);
            flash_pin(LED_PIN, 100);
            Ctrl.setControllerSettings(ctrlSettings);
            controllerSettings = ctrlSettings;
            ESP_LOGI(tag, "Fanstate: %d", ctrlSettings.fanState);
            ESP_LOGI(tag, "Flush: %d", ctrlSettings.flush);
            ESP_LOGI(tag, "Element low: %d", ctrlSettings.elementLow);
            ESP_LOGI(tag, "Element high: %d", ctrlSettings.elementHigh);
            ESP_LOGI(tag, "Prod condensor: %d", ctrlSettings.prodCondensor);
            ESP_LOGI(tag, "Controller settings updated");
        }
        
        updateTemperatures(temperatures);
        Ctrl.updatePumpSpeed(temperatures[T_head]);     // Eventually controller needs access to all temps
        vTaskDelayUntil(&xLastWakeTime, 200 / portTICK_PERIOD_MS);
    }
}

void nvs_initialize(void)
{
    // Initialize NVS
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_LOGI(tag, "NVS init failed: ESP_ERR_NVS_NO_FREE_PAGES");
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
}

ctrlParams_t getSettingsFromNVM(void)
{
    nvs_handle nvs;
    ctrlParams_t ctrlParams;
    memset(&ctrlParams, 0, sizeof(ctrlParams_t));
    int32_t setpoint, P_gain, I_gain, D_gain;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &nvs);
    if (err != ESP_OK) {
        ESP_LOGI(tag, "Error (%s) opening NVS handle!", esp_err_to_name(err));
    } else {
        ESP_LOGI(tag, "NVS handle opened successfully");
    }

    err = nvs_get_i32(nvs, "setpoint", &setpoint);
    switch (err) {
        case ESP_OK:
            ctrlParams.setpoint = (float) setpoint / 1000;
            break;
        case ESP_ERR_NVS_NOT_FOUND:
            nvs_set_i32(nvs, "setpoint", (int32_t)(ctrlParams.setpoint * 1000));
            break;
        default:
            ESP_LOGW(tag, "Error (%s) reading!\n", esp_err_to_name(err));
    }

    err = nvs_get_i32(nvs, "P_gain", &P_gain);
    switch (err) {
        case ESP_OK:
            ctrlParams.P_gain = (float) P_gain / 1000;
            break;
        case ESP_ERR_NVS_NOT_FOUND:
            nvs_set_i32(nvs, "P_gain", (int32_t)(ctrlParams.P_gain * 1000));
            break;
        default:
            ESP_LOGW(tag, "Error (%s) reading!\n", esp_err_to_name(err));
    }

    err = nvs_get_i32(nvs, "I_gain", &I_gain);
    switch (err) {
        case ESP_OK:
            ctrlParams.I_gain = (float) I_gain / 1000;
            break;
        case ESP_ERR_NVS_NOT_FOUND:
            nvs_set_i32(nvs, "I_gain", (int32_t)(ctrlParams.I_gain * 1000));
            break;
        default:
            ESP_LOGW(tag, "Error (%s) reading!\n", esp_err_to_name(err));
    }

    err = nvs_get_i32(nvs, "D_gain", &D_gain);
    switch (err) {
        case ESP_OK:
            ctrlParams.D_gain = (float) D_gain / 1000;
            break;
        case ESP_ERR_NVS_NOT_FOUND:
            nvs_set_i32(nvs, "D_gain", (int32_t)(ctrlParams.D_gain * 1000));
            break;
        default:
            ESP_LOGW(tag, "Error (%s) reading!\n", esp_err_to_name(err));
    }

    err = nvs_commit(nvs);
    if (err != ESP_OK) {
        ESP_LOGW(tag, "Error (%s) reading!\n", esp_err_to_name(err));
    }
    nvs_close(nvs);
    return ctrlParams;
}

esp_err_t updateTemperatures(float tempArray[])
{
    static float temperatures[n_tempSensors] = {0};
    if (xQueueReceive(tempQueue, temperatures, 100 / portTICK_PERIOD_MS)) {
        memcpy(tempArray, temperatures, n_tempSensors * sizeof(float));
        return ESP_OK;
    }

    memcpy(tempArray, temperatures, n_tempSensors * sizeof(float));     // If no new temps in queue, copy most recent reading
    return ESP_ERR_NOT_FOUND;
}

float get_flowRate(void)
{
    static float flowRate = 0;
    float new_flowRate;
    if (xQueueReceive(flowRateQueue, &new_flowRate, 50 / portTICK_PERIOD_MS)) {
        flowRate = new_flowRate;
    }

    return flowRate;
}

// Public access to control element states
ctrlParams_t get_controller_params(void)
{
    return controllerParams;
}

ctrlSettings_t getControllerSettings(void)
{
    return controllerSettings;
}

// Compute the partial vapour pressure in kPa based on the
// Antoine equation https://en.wikipedia.org/wiki/Antoine_equation
float computeVapourPressure(float A, float B, float C, float T)
{
    float exp = A - B / (C + T);
    return pow(10, exp) / 1000;
}

// Compute bubble line based on The Compleat Distiller
// Ch8 - Equilibrium curves
float computeLiquidEthConcentration(float temp)
{
    float P_eth = computeVapourPressure(ETH_A, ETH_B, ETH_C, temp);
    float P_H20 = computeVapourPressure(H20_A, H20_B, H20_C, temp);
    return (P_atm - P_H20) / (P_eth - P_H20);
}

// Compute dew line based on The Compleat Distiller
// Ch8 - Equilibrium curves
float computeVapourEthConcentration(float temp)
{
    float molFractionEth = computeLiquidEthConcentration(temp);
    float P_eth = computeVapourPressure(ETH_A, ETH_B, ETH_C, temp);
    return molFractionEth * P_eth / P_atm;
}

float getBoilerConcentration(float boilerTemp)
{
    float conc = -1;

    // Simple criteria for detecting if liquid is boiling or not. Valid for mashes up to ~15% (Confirm with experimental data)
    if (boilerTemp >= 90) {
        conc = computeLiquidEthConcentration(boilerTemp) * 100;
    }

    return conc;
}

float getVapourConcentration(float vapourTemp)
{
    float conc = -1;

    // Valid range for estimator model
    if (vapourTemp >= 77) {
        conc = computeVapourEthConcentration(vapourTemp) * 100;
    }

    return conc;
}

#ifdef __cplusplus
}
#endif


