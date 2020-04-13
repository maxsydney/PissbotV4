#include <stdio.h>
#include <string.h>
#include <esp_log.h>
#include <esp_err.h>
#include <string.h>
#include "messages.h"
#include "sensors.h"
#include "ds18b20.h"
#include "networking.h"
#include "controlLoop.h"
#include "cJSON.h"
#include "ds18b20.h"
#include "ota.h"

ctrlParams_t* readCtrlParams(cJSON* JSON_root)
{
    cJSON* data = cJSON_GetObjectItem(JSON_root, "data");
    ctrlParams_t* ctrlParams = malloc(sizeof(ctrlParams_t));
    ctrlParams->setpoint = cJSON_GetObjectItem(data, "setpoint")->valuedouble;
    ctrlParams->P_gain = cJSON_GetObjectItem(data, "P_gain")->valuedouble;
    ctrlParams->I_gain = cJSON_GetObjectItem(data, "I_gain")->valuedouble;
    ctrlParams->D_gain = cJSON_GetObjectItem(data, "D_gain")->valuedouble;

    return ctrlParams;    
}

ctrlSettings_t* readCtrlSettings(cJSON* JSON_root)
{
    cJSON* data = cJSON_GetObjectItem(JSON_root, "data");
    ctrlSettings_t* ctrlSettings = malloc(sizeof(ctrlSettings_t));
    ctrlSettings->fanState = cJSON_GetObjectItem(data, "fanState")->valueint;
    ctrlSettings->flush = cJSON_GetObjectItem(data, "flush")->valueint;
    ctrlSettings->elementLow = cJSON_GetObjectItem(data, "elementLow")->valueint;
    ctrlSettings->elementHigh = cJSON_GetObjectItem(data, "elementHigh")->valueint;
    ctrlSettings->prodCondensor = cJSON_GetObjectItem(data, "prodCondensor")->valueint;

    return ctrlSettings;
}

DS18B20_t readTempSensorParams(cJSON* JSON_root)
{
    DS18B20_t sens = {0};
    cJSON* data = cJSON_GetObjectItem(JSON_root, "data");
    cJSON* addr = cJSON_GetObjectItem(data, "addr");
    cJSON* byte;
    int i = 0;
    cJSON_ArrayForEach(byte, addr) {
        sens.addr.bytes[i++] = (uint8_t) byte->valueint;
    }

    sens.task = cJSON_GetObjectItem(data, "task")->valueint;

    return sens;
}