#pragma once

void temp_sensor_task(void *pvParameters);
esp_err_t sensor_init(uint8_t ds_pin);

xQueueHandle tempQueue;