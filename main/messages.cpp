#ifdef __cplusplus
extern "C" {
#endif

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
#include "webServer.h"
#include "cJSON.h"
#include "ds18b20.h"
#include "ota.h"

static const char* tag = "Messages";

esp_err_t readCtrlParams(cJSON* JSON_root, ctrlParams_t* ctrlParams)
{
    if (JSON_root == NULL) {
        ESP_LOGW(tag, "JSON root was null!");
        return ESP_FAIL;
    }
    cJSON* data = NULL;
    
    data = cJSON_GetObjectItem(JSON_root, "data");

    if (cJSON_IsObject(data)) {
        cJSON* setPoint = cJSON_GetObjectItem(data, "setpoint");
        if (cJSON_IsNumber(setPoint)) {
            ctrlParams->setpoint = setPoint->valuedouble;
        } else {
            ESP_LOGW(tag, "Unable to read setpoint");
            return ESP_FAIL;
        }

        cJSON* P_gain = cJSON_GetObjectItem(data, "P_gain");
        if (cJSON_IsNumber(P_gain)) {
            ctrlParams->P_gain = P_gain->valuedouble;
        } else {
            ESP_LOGW(tag, "Unable to read P gain");
            return ESP_FAIL;
        }

        cJSON* I_gain = cJSON_GetObjectItem(data, "I_gain");
        if (cJSON_IsNumber(I_gain)) {
            ctrlParams->I_gain = I_gain->valuedouble;
        } else {
            ESP_LOGW(tag, "Unable to read I gain");
            return ESP_FAIL;
        }

        cJSON* D_gain = cJSON_GetObjectItem(data, "D_gain");
        if (cJSON_IsNumber(D_gain)) {
            ctrlParams->D_gain = D_gain->valuedouble;
        } else {
            ESP_LOGW(tag, "Unable to read D gain");
            return ESP_FAIL;
        }

        cJSON* LPFCutoff = cJSON_GetObjectItem(data, "LPFCutoff");
        if (cJSON_IsNumber(LPFCutoff)) {
            ctrlParams->LPFCutoff = LPFCutoff->valuedouble;
        } else {
            ESP_LOGW(tag, "Unable to read LPFCutoff");
            return ESP_FAIL;
        }
    } else {
        return ESP_FAIL;
    }

    return ESP_OK;    
}

esp_err_t readCtrlSettings(cJSON* JSON_root, ctrlSettings_t* ctrlSettings)
{
    if (JSON_root == NULL) {
        ESP_LOGW(tag, "JSON root was null!");
        return ESP_FAIL;
    }

    cJSON* data = NULL;
    data = cJSON_GetObjectItem(JSON_root, "data");

    if (cJSON_IsObject(data)) {
        cJSON* fanState = cJSON_GetObjectItem(data, "fanState");
        if (cJSON_IsNumber(fanState)) {
            ctrlSettings->fanState = fanState->valueint;
        } else {
            ESP_LOGW(tag, "Unable to read fan state");
            return ESP_FAIL;
        }

        cJSON* flush = cJSON_GetObjectItem(data, "flush");
        if (cJSON_IsNumber(flush)) {
            ctrlSettings->flush = flush->valueint;
        } else {
            ESP_LOGW(tag, "Unable to read flush state");
            return ESP_FAIL;
        }

        cJSON* elementLow = cJSON_GetObjectItem(data, "elementLow");
        if (cJSON_IsNumber(elementLow)) {
            ctrlSettings->elementLow = elementLow->valueint;
        } else {
            ESP_LOGW(tag, "Unable to read 2.4kW element state");
            return ESP_FAIL;
        }

        cJSON* elementHigh = cJSON_GetObjectItem(data, "elementHigh");
        if (cJSON_IsNumber(elementHigh)) {
            ctrlSettings->elementHigh = elementHigh->valueint;
        } else {
            ESP_LOGW(tag, "Unable to read 3.0kW element state");
            return ESP_FAIL;
        }

        cJSON* prodCondensor = cJSON_GetObjectItem(data, "prodCondensor");
        if (cJSON_IsNumber(prodCondensor)) {
            ctrlSettings->prodCondensor = prodCondensor->valueint;
        } else {
            ESP_LOGW(tag, "Unable to read prodCondensor pump state");
            return ESP_FAIL;
        }
    } else {
        ESP_LOGW(tag, "Could not read JSON object");
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t readTempSensorParams(cJSON* JSON_root, DS18B20_t* sens)
{
    if (JSON_root == NULL) {
        ESP_LOGW(tag, "JSON root was null!");
        return ESP_FAIL;
    }

    cJSON* data = NULL;
    cJSON* addr = NULL;
    
    data = cJSON_GetObjectItem(JSON_root, "data");

    if (cJSON_IsObject(data)) {
        addr = cJSON_GetObjectItem(data, "addr");
        if (cJSON_IsArray(addr)) {
            cJSON* byte = NULL;
            int i = 0;
            cJSON_ArrayForEach(byte, addr) {
                if (cJSON_IsNumber(byte)) {
                    sens->addr.bytes[i++] = (uint8_t) byte->valueint;
                } else {
                    ESP_LOGW(tag, "DS18B20 address corrupt");
                    return ESP_FAIL;
                }
            }
        } else {
            ESP_LOGW(tag, "Could not read sensor address");
            return ESP_FAIL;
        }
    } else {
        ESP_LOGW(tag, "Could not read JSON object");
        return ESP_FAIL;
    }

    cJSON* task = cJSON_GetObjectItem(data, "task");
    if (cJSON_IsNumber(task)) {
        sens->task = static_cast<tempSensor>(cJSON_GetObjectItem(data, "task")->valueint);
    } else {
        ESP_LOGW(tag, "Could not determine sensor task");
        return ESP_FAIL;
    }

    return ESP_OK;
}

int _log_vprintf(const char *fmt, va_list args) 
{
    static bool static_fatal_error = false;
    char logBuff[512];
    int iresult;

    if (static_fatal_error == false) {
        iresult = vsnprintf(logBuff, 511, fmt, args);
        if (iresult < 0) {
            printf("%s() ABORT. failed vfprintf() -> disable future vfprintf(_log_remote_fp) \n", __FUNCTION__);
            // MARK FATAL
            static_fatal_error = true;
            return iresult;
        }

        // Log to websocket (if available)
        wsLog(logBuff);
    }

    // #3 ALWAYS Write to stdout!
    return vprintf(fmt, args);
}

#ifdef __cplusplus
}
#endif
