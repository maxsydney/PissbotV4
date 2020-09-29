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
#include "DistillerManager.h"

static const char* tag = "Main";

void app_main()
{
    // Initialise peripherals and drivers
    nvs_flash_init();
    nvs_initialize();
    uart_initialize();
    init_timer();
    gpio_init();
    wifi_connect();
    LCD_init(LCD_ADDR, LCD_SDA, LCD_SCL, LCD_COLS, LCD_ROWS);
    sensor_init(ONEWIRE_BUS, DS18B20_RESOLUTION_11_BIT);
    controller_init(CONTROL_LOOP_FREQUENCY);
    webServer_init();
    init_input();

    ESP_LOGI(tag, "Redirecting log messages to websocket connection");
    esp_log_set_vprintf(&_log_vprintf);
    
    DistillerConfig cfg {};
    cfg.ctrlConfig.updateFreqHz = CONTROL_LOOP_FREQUENCY;
    cfg.ctrlConfig.refluxPumpCfg = PumpCfg(REFLUX_PUMP, LEDC_CHANNEL_0, LEDC_TIMER_0);
    cfg.ctrlConfig.prodPumpCfg = PumpCfg(PROD_PUMP, LEDC_CHANNEL_1, LEDC_TIMER_1);
    cfg.ctrlConfig.fanPin = FAN_SWITCH;
    cfg.ctrlConfig.element1Pin = ELEMENT_1;
    cfg.ctrlConfig.element2Pin = ELEMENT_2;
    
    DistillerManager* manager = DistillerManager::getInstance(5, 8192, 1, cfg);
    manager->begin();
    
    // Schedule tasks
    xTaskCreatePinnedToCore(&temp_sensor_task, "Temperature Sensor", 16384, NULL, 7, NULL, 1);
    // xTaskCreatePinnedToCore(&flowmeter_task, "Flowrate", 2048, NULL, 7, NULL, 1);
    xTaskCreatePinnedToCore(&control_loop, "Controller", 16384, NULL, 7, NULL, 0);
    // xTaskCreatePinnedToCore(&menu_task, "LCD task", 2048, NULL, 3, NULL, 0);
    xTaskCreatePinnedToCore(&inputButtonTask, "Input button task", 16384, NULL, 5, NULL, 1);
    xTaskCreatePinnedToCore(&heartBeatTask, "Heartbeat task", 16384, NULL, 1, NULL, 1);
}

#ifdef __cplusplus
}
#endif