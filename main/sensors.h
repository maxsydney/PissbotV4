#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

xQueueHandle hotSideTempQueue;
xQueueHandle coldSideTempQueue;
xQueueHandle flowRateQueue;

void temp_sensor_task(void *pvParameters);
esp_err_t sensor_init(uint8_t ds_pin);
void flowmeter_task(void *pvParameters);
void IRAM_ATTR flowmeter_ISR(void* arg);

typedef enum {
    coldSideSensor,
    hotSideSensor
} tempSensor;

