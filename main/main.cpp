#ifdef __cplusplus
extern "C" {
#endif

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
#include "controlLoop.h"
#include "pinDefs.h"
#include "HD44780.h"
#include "LCD.h"
#include "webServer.h"
#include "input.h"
#include "menu.h"

void app_main()
{
    // Initialise peripherals and drivers
    nvs_flash_init();
    sensor_init(ONEWIRE_BUS, DS18B20_RESOLUTION_11_BIT);
    nvs_initialize();
    uart_initialize();
    init_timer();
    gpio_init();
    wifi_connect();
    // initPumps();
    controller_init(CONTROL_LOOP_FREQUENCY);
    LCD_init(LCD_ADDR, LCD_SDA, LCD_SCL, LCD_COLS, LCD_ROWS);
    webServer_init();
    init_input();
    
    // Schedule tasks
    xTaskCreatePinnedToCore(&temp_sensor_task, "Temperature Sensor", 4096, NULL, 7, NULL, 1);
    // xTaskCreatePinnedToCore(&flowmeter_task, "Flowrate", 2048, NULL, 7, NULL, 1);
    xTaskCreatePinnedToCore(&control_loop, "Controller", 8192, NULL, 6, NULL, 0);
    xTaskCreatePinnedToCore(&menu_task, "LCD task", 2048, NULL, 3, NULL, 0);
}

#ifdef __cplusplus
}
#endif