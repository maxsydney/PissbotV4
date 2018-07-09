#include <stdio.h>
#include <stdlib.h>
#include <esp_log.h>
#include <esp_err.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "ds18b20.h" 
#include "sensors.h"
#include "main.h"

const char* tag = "Sensors";

esp_err_t sensor_init(uint8_t ds_pin)
{
    ds18b20_init(ds_pin);
    set_resolution_10_bit();
    hotSideTempQueue = xQueueCreate(2, sizeof(float));
    coldSideTempQueue = xQueueCreate(2, sizeof(float));
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
            hotTemp = newHotTemp;
        }

        if (newColdTemp != 0) {
            coldTemp = newColdTemp;
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