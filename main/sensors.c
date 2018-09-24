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
#include "main.h"

const char* tag = "Sensors";

static volatile double timeVal;
static float filter_singlePoleIIR(float x, float y, float alpha);
static float FIR_filter_lowpass(float x, tempSensor channel);

const float FIRcoeff[5] = {0.02840647, 0.23700821, 0.46917063, 0.23700821, 0.02840647};
static float sensorBuffer[2][9];
static bool filterTemp;

esp_err_t sensor_init(uint8_t ds_pin)
{
    ds18b20_init(ds_pin);
    filterTemp = false;
    // set_resolution_10_bit();
    hotSideTempQueue = xQueueCreate(2, sizeof(float));
    coldSideTempQueue = xQueueCreate(2, sizeof(float));
    flowRateQueue = xQueueCreate(2, sizeof(float));
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
    float hotTemp = ds18b20_get_temp(hotSideSensor);
    float coldTemp = ds18b20_get_temp(coldSideSensor);
    float newHotTemp, newColdTemp;
    BaseType_t ret;
    while (1) 
    {
        newHotTemp = ds18b20_get_temp(hotSideSensor);
        newColdTemp = ds18b20_get_temp(coldSideSensor);

        if (newHotTemp != 0) {
            
            if (filterTemp) {

                hotTemp = FIR_filter_lowpass(newHotTemp, hotSideSensor);
                // hotTemp = filter_singlePoleIIR(newHotTemp, hotTemp, 0.2);
            } else {
                hotTemp = newHotTemp;
            } 
        }

        if (newColdTemp != 0) {
            if (filterTemp) {
                coldTemp = filter_singlePoleIIR(newColdTemp, coldTemp, 0.3);
            } else {
                coldTemp = newColdTemp;
            }
        }

        ret = xQueueSend(hotSideTempQueue, &hotTemp, 100);
        if (ret == errQUEUE_FULL) {
            ESP_LOGI(tag, "Hot side temp queue full");
        }

        ret = xQueueSend(coldSideTempQueue, &coldTemp, 100);
        if (ret == errQUEUE_FULL) {
            ESP_LOGI(tag, "Cold side temp queue full");
        }
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
        ret = xQueueSend(flowRateQueue, &flowRate, 100);
        if (ret == errQUEUE_FULL) {
            ESP_LOGI(tag, "Flow rate queue full");
        }
        vTaskDelayUntil(&xLastWakeTime, 250 / portTICK_PERIOD_MS);     // Run every second
    }

}

static float filter_singlePoleIIR(float x, float y, float alpha)
{
    return alpha * x + (1 - alpha) * y; 
}

void IRAM_ATTR flowmeter_ISR(void* arg)
{
    double timeTemp;
    timer_get_counter_time_sec(TIMER_GROUP_0, TIMER_0, &timeTemp);
    timer_set_counter_value(TIMER_GROUP_0, TIMER_0, 0x00000000ULL);
    timeVal = timeTemp;
}

static float FIR_filter_lowpass(float x, tempSensor channel)
{
    static float data[2][5] = {0};
    static int arrPtr[2] = {0};
    int j = arrPtr[channel]++;
    float output = 0;

    data[channel][j] = x;

    for (int i = 4; i >= 0; i--) {
        output += data[channel][j--] * FIRcoeff[i];
        if (j < 0) {
            j = 4;
        }
    }

    if (arrPtr[channel] > 4) {
        arrPtr[channel] = 0;
    }

    return output;
}

void setTempFilter(bool status)
{
    filterTemp = status;
}