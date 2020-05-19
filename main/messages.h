#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "controlLoop.h"
#include "sensors.h"
#include "cJSON.h"

/*
*   --------------------------------------------------------------------  
*   readCtrlParams
*   --------------------------------------------------------------------
*   Decodes incoming messages with type INFO and subtype ctrlParams. 
*/
esp_err_t readCtrlParams(cJSON* JSON_root, ctrlParams_t* ctrlParams);

/*
*   --------------------------------------------------------------------  
*   readCtrlSettings
*   --------------------------------------------------------------------
*   Decodes incoming messages with type INFO and subtype ctrlSettings. 
*/
esp_err_t readCtrlSettings(cJSON* JSON_root, ctrlSettings_t* ctrlSettings);

esp_err_t readTempSensorParams(cJSON* JSON_root, DS18B20_t* sens);

#ifdef __cplusplus
}
#endif
