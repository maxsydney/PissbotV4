// #ifdef __cplusplus
// extern "C" {
// #endif

#include <stdio.h>
#include <esp_log.h>
#include <esp_event.h>
#include <esp_timer.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "main.h"
#include "sdkconfig.h"
#include "pinDefs.h"
#include "webServer.h"
#include "DistillerManager.h"

static const char* tag = "Main";

#ifdef __cplusplus
extern "C" {
#endif

void app_main()
{
    // ESP_LOGI(tag, "Redirecting log messages to websocket connection");
    // esp_log_set_vprintf(&_log_vprintf);
    
    DistillerConfig cfg {};
    cfg.ctrlConfig.dt = 1.0 / CONTROL_LOOP_FREQUENCY;
    cfg.ctrlConfig.refluxPumpCfg = PumpCfg(REFLUX_PUMP, LEDC_CHANNEL_0, LEDC_TIMER_0);
    cfg.ctrlConfig.prodPumpCfg = PumpCfg(PROD_PUMP, LEDC_CHANNEL_1, LEDC_TIMER_1);
    cfg.ctrlConfig.fanPin = FAN_SWITCH;
    cfg.ctrlConfig.element1Pin = ELEMENT_1;
    cfg.ctrlConfig.element2Pin = ELEMENT_2;

    cfg.sensorManagerConfig.dt = 1.0 / CONTROL_LOOP_FREQUENCY;
    cfg.sensorManagerConfig.oneWireConfig.oneWirePin = ONEWIRE_BUS;
    cfg.sensorManagerConfig.oneWireConfig.refluxFlowPin = REFLUX_FLOW;
    cfg.sensorManagerConfig.oneWireConfig.productFlowPin = PROD_FLOW;
    cfg.sensorManagerConfig.oneWireConfig.tempSensorResolution = DS18B20_RESOLUTION_11_BIT;

    DistillerManager* manager = DistillerManager::getInstance(5, 8192, 1, cfg);
    manager->begin();
}

#ifdef __cplusplus
}
#endif