#pragma once
xQueueHandle hotSideTempQueue;
xQueueHandle coldSideTempQueue;

void temp_sensor_task(void *pvParameters);
esp_err_t sensor_init(uint8_t ds_pin);

typedef enum {
    coldSideSensor,
    hotSideSensor
} tempSensor;

