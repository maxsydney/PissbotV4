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
static bool element_status = 0, flushSystem = 0;
uint16_t ctrl_loop_period_ms;
static int fanState = 0;

static Data controllerSettings;

esp_err_t controller_init(uint8_t frequency)
{
    dataQueue = xQueueCreate(2, sizeof(Data));
    ctrl_loop_period_ms = 1.0 / frequency * 1000;
    flushSystem = false;
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

void nvs_initialize(void)
{
    // Initialize NVS
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
}

void control_loop(void* params)
{
    static double output;
    double error, derivative;
    double last_error = 0;
    double integral = 0;
    float temperatures[n_tempSensors] = {0};
    float loopPeriodSeconds;
    portTickType xLastWakeTime = xTaskGetTickCount();

    while(1) {
        loopPeriodSeconds = ctrl_loop_period_ms / 1000;
        if (uxQueueMessagesWaiting(dataQueue)) {
            xQueueReceive(dataQueue, &controllerSettings, 50 / portTICK_PERIOD_MS);
            flash_pin(LED_PIN, 100);
            ESP_LOGI(tag, "%s\n", "Received data from queue");
        }
        
        getTemperatures(temperatures);
        checkFan(temperatures[T_refluxHot]);

        error =  temperatures[T_refluxHot] - controllerSettings.setpoint;
        derivative = (error - last_error) / 0.2;
        last_error = error;

        // Basic strategy to avoid integral windup. Only integrate when output is not saturated
        if ((output > SENSOR_MAX_OUTPUT - 1) && (error > 0)) {
            integral += 0;
        } else if ((output < SENSOR_MIN_OUTPUT + 1) && (error < 0)) {
            integral += 0;
        } else {
            integral += error * ctrl_loop_period_ms / 1000;                                       
        }
                        
        output = controllerSettings.P_gain * error + controllerSettings.D_gain * derivative + controllerSettings.I_gain * integral;

        // Clip output
        if (output < SENSOR_MIN_OUTPUT) {
            output = SENSOR_MIN_OUTPUT;      // Ensures water is always flowing so sensor can get a reading
        } else if (output > SENSOR_MAX_OUTPUT) {
            output = SENSOR_MAX_OUTPUT;
        }
       

        if (flushSystem) {
            output = 5000;
        }

        set_motor_speed(output);
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


