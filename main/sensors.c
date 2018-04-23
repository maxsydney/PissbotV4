#include <stdio.h>
#include <stdlib.h>
#include <esp_log.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "ds18b20.h" 
#include "sensors.h"
#include "main.h"

esp_err_t sensor_init(uint8_t ds_pin)
{
    ds18b20_init(ds_pin);
    set_resolution_10_bit();
    tempQueue = xQueueCreate(2, sizeof(float));
    return ESP_OK;
}

void temp_sensor_task(void *pvParameters) 
{
    float temp = ds18b20_get_temp();
    float newTemp;
    while (1) 
    {
        newTemp = ds18b20_get_temp();

        if (newTemp != 0) {
            temp = newTemp;
        }

        BaseType_t ret = xQueueSend(tempQueue, &temp, 500);
        if (ret == errQUEUE_FULL) {
            // printf("Queue full\n");
        } else if (ret == pdTRUE) {
            // printf("Successfully enqueued temp\n");
        }
    }
}