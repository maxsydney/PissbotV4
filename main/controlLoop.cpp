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

static char tag[] = "Controller";
static bool element1_status = 0, element2_status = 0, flushSystem = 0, prodManual = 0;
static int fanState = 0;

static Data controllerSettings;
xQueueHandle dataQueue;
xQueueHandle cmdQueue;
uint16_t ctrl_loop_period_ms;

esp_err_t controller_init(uint8_t frequency)
{
    dataQueue = xQueueCreate(10, sizeof(Data));
    cmdQueue = xQueueCreate(10, sizeof(Cmd_t));
    ctrl_loop_period_ms = 1.0 / frequency * 1000;
    flushSystem = false;

    return ESP_OK;
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

Data getSettingsFromNVM(void)
{
    nvs_handle nvs;
    Data settings;
    memset(&settings, 0, sizeof(Data));
    int32_t setpoint, P_gain, I_gain, D_gain;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &nvs);
    if (err != ESP_OK) {
        ESP_LOGI(tag, "Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    } else {
        ESP_LOGI(tag, "NVS handle opened successfully\n");
    }

    err = nvs_get_i32(nvs, "setpoint", &setpoint);
    switch (err) {
        case ESP_OK:
            settings.setpoint = (float) setpoint / 1000;
            break;
        case ESP_ERR_NVS_NOT_FOUND:
            nvs_set_i32(nvs, "setpoint", (int32_t)(settings.setpoint * 1000));
            break;
        default:
            ESP_LOGW(tag, "Error (%s) reading!\n", esp_err_to_name(err));
    }

    err = nvs_get_i32(nvs, "P_gain", &P_gain);
    switch (err) {
        case ESP_OK:
            settings.P_gain = (float) P_gain / 1000;
            break;
        case ESP_ERR_NVS_NOT_FOUND:
            nvs_set_i32(nvs, "P_gain", (int32_t)(settings.P_gain * 1000));
            break;
        default:
            ESP_LOGW(tag, "Error (%s) reading!\n", esp_err_to_name(err));
    }

    err = nvs_get_i32(nvs, "I_gain", &I_gain);
    switch (err) {
        case ESP_OK:
            settings.I_gain = (float) I_gain / 1000;
            break;
        case ESP_ERR_NVS_NOT_FOUND:
            nvs_set_i32(nvs, "I_gain", (int32_t)(settings.I_gain * 1000));
            break;
        default:
            ESP_LOGW(tag, "Error (%s) reading!\n", esp_err_to_name(err));
    }

    err = nvs_get_i32(nvs, "D_gain", &D_gain);
    switch (err) {
        case ESP_OK:
            settings.D_gain = (float) D_gain / 1000;
            break;
        case ESP_ERR_NVS_NOT_FOUND:
            nvs_set_i32(nvs, "D_gain", (int32_t)(settings.D_gain * 1000));
            break;
        default:
            ESP_LOGW(tag, "Error (%s) reading!\n", esp_err_to_name(err));
    }

    err = nvs_commit(nvs);
    if (err != ESP_OK) {
        ESP_LOGW(tag, "Error (%s) reading!\n", esp_err_to_name(err));
    }
    nvs_close(nvs);
    return settings;
}

void control_loop(void* params)
{
    float temperatures[n_tempSensors] = {0};
    Data settings = getSettingsFromNVM();
    controllerSettings= settings;
    Cmd_t cmdSettings;
    Controller Ctrl = Controller(CONTROL_LOOP_FREQUENCY, settings, REFLUX_PUMP, LEDC_CHANNEL_0, LEDC_TIMER_0, PROD_PUMP, LEDC_CHANNEL_1, LEDC_TIMER_1, FAN_SWITCH, ELEMENT_1, ELEMENT_2);
    portTickType xLastWakeTime = xTaskGetTickCount();

    
    while (true) {
        if (uxQueueMessagesWaiting(dataQueue)) {
            xQueueReceive(dataQueue, &controllerSettings, 50 / portTICK_PERIOD_MS);
            flash_pin(LED_PIN, 100);
            Ctrl.setControllerSettings(controllerSettings);
            ESP_LOGI(tag, "%s\n", "Received data from queue");
        }

        if (uxQueueMessagesWaiting(cmdQueue)) {
            xQueueReceive(cmdQueue, &cmdSettings, 50 / portTICK_PERIOD_MS);
            flash_pin(LED_PIN, 100);
            Ctrl.processCommand(cmdSettings);
            fanState = Ctrl.getFanState();
            element1_status = Ctrl.getElem24State();
            element2_status = Ctrl.getElem3State();
            flushSystem = Ctrl.getFlush();
            prodManual = Ctrl.getProdManual();
            ESP_LOGI(tag, "%s\n", "Received command from queue");
        }
        
        getTemperatures(temperatures);
        checkFan(temperatures[T_refluxHot]);

        Ctrl.updatePumpSpeed(temperatures[0]);
        ESP_LOGI("wifi", "free Heap:%zu,%zu", esp_get_free_heap_size(), heap_caps_get_free_size(MALLOC_CAP_8BIT));
        vTaskDelayUntil(&xLastWakeTime, 200 / portTICK_PERIOD_MS);
    }
}

esp_err_t getTemperatures(float tempArray[])
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
float get_setpoint(void)
{
    return controllerSettings.setpoint;
}

bool get_element1_status(void)
{
    return element1_status;
}

bool get_element2_status(void)
{
    return element2_status;
}

bool get_productCondensorManual(void)
{
    return prodManual;
}

Data get_controller_settings(void)
{
    return controllerSettings;
}

bool get_fan_state(void)
{
    return fanState;
}

bool getFlush(void)
{
    return flushSystem;
}

void setFanState(int state)
{
    if (state) {
        printf("Switching fan on\n");
        setPin(FAN_SWITCH, state);
    } else {
        printf("Switching fan off\n");
        setPin(FAN_SWITCH, state);
    }
    fanState = state;
}

void setFlush(bool state)
{
    printf("setFlush called with state %d\n", state);
    flushSystem = state;
}

void checkFan(double T1)
{
    if (T1 > FAN_THRESH && !fanState) {
        setPin(FAN_SWITCH, 1);
        fanState = 1;
    } else if (T1 <= FAN_THRESH) {
        setPin(FAN_SWITCH, 0);
        fanState = 0;
    }
}

void setElementState(int state)
{
    if (state) {
        printf("Switching element on\n");
        setPin(ELEMENT_1, state);
    } else {
        printf("Switching element off\n");
        setPin(ELEMENT_1, state);
    }
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


