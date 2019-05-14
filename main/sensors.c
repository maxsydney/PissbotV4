#include <stdio.h>
#include <stdlib.h>
#include <esp_log.h>
#include <esp_err.h>
#include <esp_timer.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "driver/timer.h"
#include "ds18b20.h" 
#include "sensors.h"
#include "controller.h"
#include "main.h"
#include "pinDefs.h"

static const char* tag = "Sensors";

static volatile double timeVal;

const float FIRcoeff[5] = {0.02840647, 0.23700821, 0.46917063, 0.23700821, 0.02840647};
static float sensorBuffer[2][9];
static bool filterTemp;

esp_err_t sensor_init(uint8_t ds_pin)
{
    ds18b20_init(ds_pin);
    filterTemp = false;
    tempQueue = xQueueCreate(10, sizeof(float[5]));
    hotSideTempQueue = xQueueCreate(10, sizeof(float));
    coldSideTempQueue = xQueueCreate(10, sizeof(float));
    flowRateQueue = xQueueCreate(10, sizeof(float));

    checkPowerSupply();
    return ESP_OK;
}

esp_err_t init_timer(void)
{
    timer_config_t config;
    config.divider = 2;
    config.counter_dir = TIMER_COUNT_UP;
    config.alarm_en = TIMER_ALARM_DIS;
    timer_init(TIMER_GROUP_0, TIMER_0, &config);

    timer_set_counter_value(TIMER_GROUP_0, TIMER_0, 0x00000000ULL);
    timer_start(TIMER_GROUP_0, TIMER_0);

    return ESP_OK;
}

void temp_sensor_task(void *pvParameters) 
{
    float sensorTemps[5] = {0};
    portTickType xLastWakeTime = xTaskGetTickCount();
    BaseType_t ret;

    while (1) 
    {
        readTemps(sensorTemps);
        ret = xQueueSend(tempQueue, sensorTemps, 100 / portTICK_PERIOD_MS); 
        if (ret == errQUEUE_FULL) {
            ESP_LOGI(tag, "Flow rate queue full");
        }
        vTaskDelayUntil(&xLastWakeTime, ctrl_loop_period_ms / portTICK_PERIOD_MS);
    }
}

void flowmeter_task(void *pvParameters) 
{
    float flowRate;
    BaseType_t ret;
    portTickType xLastWakeTime = xTaskGetTickCount();
    double currTime;

    while (true) {
        timer_get_counter_time_sec(TIMER_GROUP_0, TIMER_0, &currTime);
        if (currTime > 1) {
            flowRate = 0;
        } else {
            flowRate = (float) 1 / (timeVal * 7.5);
        }
        ret = xQueueSend(flowRateQueue, &flowRate, 100 / portTICK_PERIOD_MS);
        if (ret == errQUEUE_FULL) {
            ESP_LOGI(tag, "Flow rate queue full");
        }
        vTaskDelayUntil(&xLastWakeTime, ctrl_loop_period_ms / portTICK_PERIOD_MS);     // Run every second
    }
}

void IRAM_ATTR flowmeter_ISR(void* arg)
{
    double timeTemp;
    timer_get_counter_time_sec(TIMER_GROUP_0, TIMER_0, &timeTemp);
    timer_set_counter_value(TIMER_GROUP_0, TIMER_0, 0x00000000ULL);
    timeVal = timeTemp;
}

void setTempFilter(bool status)
{
    filterTemp = status;
}

void readTemps(float sensorTemps[])
{
    initiateConversion();
    vTaskDelay(200 / portTICK_RATE_MS);

    for (int i = 0; i < sensorCount; i++) {
        float T = ds18b20_get_temp(i);
        if (T != 0) {
            sensorTemps[i] = T;
        }
    }
}

void checkPowerSupply(void)
{
    for (int i = 0; i < 4; i++) {
        int supply = readPowerSupply(i);
        if (supply) {
            ESP_LOGI(tag, "Sensor on channel %d using external power", i);
        } else {
            ESP_LOGI(tag, "Sensor on channel %d using parasite power", i);
        }
    }
}