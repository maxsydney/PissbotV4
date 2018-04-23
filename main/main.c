#include <stdio.h>
#include <esp_log.h>
#include <esp_event.h>
#include <esp_timer.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "nvs_flash.h"
#include "networking.h"
#include "main.h"
#include "gpio.h"
#include "pump.h"
#include "sensors.h"
#include "messages.h"
#include "sdkconfig.h"
#include "lcd.h"
#include "controller.h"

#define DS_PIN                   15
#define CONTROL_LOOP_FREQUENCY   1 

void app_main()
{
    nvs_flash_init();
    nvs_initialize();
    sensor_init(DS_PIN);
    gpio_init();
    wifi_connect();
    pwm_init();
    controller_init(CONTROL_LOOP_FREQUENCY);

    xTaskCreate(&socket_server_task, "Socket Server", 2048, NULL, 5, NULL);
    xTaskCreatePinnedToCore(&temp_sensor_task, "Temperature Sensor", 2048, NULL, 3, NULL, 0);
    xTaskCreate(&control_loop, "Controller", 2048, NULL, 6, NULL);
}