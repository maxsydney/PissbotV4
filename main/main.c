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
#include "controller.h"
#include "pinDefs.h"
#include "HD44780.h"
#include "LCD.h"

#define CONTROL_LOOP_FREQUENCY   5

void app_main()
{
    nvs_flash_init();
    sensor_init(ONEWIRE_BUS);
    nvs_initialize();
    uart_initialize();
    init_timer();
    gpio_init();
    wifi_connect();
    pwm_init();
    controller_init(CONTROL_LOOP_FREQUENCY);
    LCD_init(LCD_ADDR, LCD_SDA, LCD_SCL, LCD_COLS, LCD_ROWS);
    

    // xTaskCreate(&socket_server_task, "Socket Server", 2048, NULL, 5, NULL);
    xTaskCreatePinnedToCore(&temp_sensor_task, "Temperature Sensor", 2048, NULL, 7, NULL, 1);
    xTaskCreatePinnedToCore(&control_loop, "Controller", 2048, NULL, 6, NULL, 0);
    // xTaskCreate(&sendDataUART, "UART send", 2048, NULL, 6, NULL);
    // xTaskCreate(&recvDataUART, "UART receive", 2048, NULL, 6, NULL);
    // xTaskCreate(&flowmeter_task, "Flow meter", 2048, NULL, 3, NULL);
    xTaskCreatePinnedToCore(&LCD_DemoTask, "LCD task", 2048, NULL, 3, NULL, 0);
}

