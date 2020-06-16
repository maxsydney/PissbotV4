#ifdef __cplusplus
extern "C" {
#endif

#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "libesphttpd/httpd-espfs.h"
#include "espfs.h"
#include "espfs_image.h"
#include "libesphttpd/httpd-freertos.h"
#include "libesphttpd/route.h"
#include "messages.h"
#include "networking.h"
#include "main.h"
#include "ota.h"
#include "webServer.h"
#include "sensors.h"
#include "connectionManager.h"

#define LISTEN_PORT     80u
#define STATIC_IP		"192.168.1.202"
#define SUBNET_MASK		"255.255.255.0"
#define GATE_WAY		"192.168.1.1"
#define DNS_SERVER		"8.8.8.8"

static char connectionMemory[sizeof(RtosConnType) * ConnectionManager::MAX_CONNECTIONS];
static const char *tag = "Webserver";
static HttpdFreertosInstance httpdFreertosInstance;
static SemaphoreHandle_t wsSemaphore = NULL;
xTaskHandle assignSensorHandle = NULL;

static void sensorAssignTask(void *pvParameters);
static void cleanJSONString(char* inputBuffer, char* destBuffer);
static void closeConnection(Websock *ws);
static void openConnection(Websock *ws);
static void sendStates(Websock* ws);

void websocket_task(void *pvParameters) 
{
    Websock *ws = (Websock*) pvParameters;
    cJSON *root;
    float temps[n_tempSensors] = {0};
    float flowRate;
    ctrlParams_t ctrlParams;
    int64_t uptime_uS;

    while (true) {
        updateTemperatures(temps);
        root = cJSON_CreateObject();
        ctrlParams = get_controller_params();
        uptime_uS = esp_timer_get_time() / 1000000;
        flowRate = get_flowRate();

        // Construct JSON object
        cJSON_AddStringToObject(root, "type", "data");
        cJSON_AddNumberToObject(root, "T_head", getTemperature(temps, T_head));
        cJSON_AddNumberToObject(root, "T_reflux", getTemperature(temps, T_boiler));
        cJSON_AddNumberToObject(root, "T_prod", getTemperature(temps, T_prod));
        cJSON_AddNumberToObject(root, "T_radiator", getTemperature(temps, T_radiator));
        cJSON_AddNumberToObject(root, "T_reflux", getTemperature(temps, T_reflux));
        cJSON_AddNumberToObject(root, "setpoint", ctrlParams.setpoint);
        cJSON_AddNumberToObject(root, "P_gain", ctrlParams.P_gain);
        cJSON_AddNumberToObject(root, "I_gain", ctrlParams.I_gain);
        cJSON_AddNumberToObject(root, "D_gain", ctrlParams.D_gain);
        cJSON_AddNumberToObject(root, "uptime", uptime_uS);
        cJSON_AddNumberToObject(root, "flowrate", flowRate);
        cJSON_AddNumberToObject(root, "boilerConc", getBoilerConcentration(getTemperature(temps, T_reflux)));
        cJSON_AddNumberToObject(root, "vapourConc", getVapourConcentration(getTemperature(temps, T_head)));
        char* JSONptr = cJSON_Print(root);
        cJSON_Delete(root);

        if (wsSemaphore != NULL) {
            if(xSemaphoreTake(wsSemaphore, (TickType_t) 10 ) == pdTRUE) {
                if (ConnectionManager::checkConnection(ws) == ESP_OK) {
                    if (cgiWebsocketSend(&httpdFreertosInstance.httpdInstance, ws, JSONptr, strlen(JSONptr), WEBSOCK_FLAG_NONE) == WEBSOCK_CLOSED) {
                        // Should never get here, but just in case
                        free(JSONptr);
                        xSemaphoreGive(wsSemaphore);
                        vTaskDelete(NULL);
                    } else {
                        // Sent successfully
                        xSemaphoreGive(wsSemaphore);
                    }
                } else {
                    // Connection has been closed. Quit task
                    free(JSONptr);
                    xSemaphoreGive(wsSemaphore);
                    vTaskDelete(NULL);
                }
            } else {
                ESP_LOGI(tag, "Unable to access websocket shared resource to send data");
            }
        }

        free(JSONptr);
        vTaskDelay(250 / portTICK_PERIOD_MS);
    }
}

static void cleanJSONString(char* inputBuffer, char* destBuffer)
{
    int idx = 0;
    for (int i=0; i < strlen(inputBuffer); i++) {
        if (inputBuffer[i] != '\\') {
            destBuffer[idx++] = inputBuffer[i];
        }
    }
    destBuffer[idx] = '\0';
}

static void myWebsocketRecv(Websock *ws, char *data, int len, int flags) {
    char* type = NULL;
    char* subType = NULL;
    char msgBuffer[len] = {};
    char cleanJSON[len] = {};
    memcpy(msgBuffer, ++data, len-2);        // Drop leading "
    msgBuffer[len-2] = '\0';

    cleanJSONString(msgBuffer, cleanJSON);
    cJSON* root = cJSON_Parse(cleanJSON);

    if (root != NULL) {
        type = cJSON_GetObjectItem(root, "type")->valuestring;
        subType = cJSON_GetObjectItem(root, "subType")->valuestring;
    } else {
        ESP_LOGW(tag, "Unable to parse JSON string");
    }

    if (type != NULL) {
        ESP_LOGI(tag, "Type is: %s", type);
    }

    if (subType != NULL) {
        ESP_LOGI(tag, "Subtype is: %s", subType);
    }

    if (strncmp(type, "INFO", 4) == 0) { 
        // Hand new data packet to controller    
        if (strncmp(subType, "ctrlParams", 10) == 0) {
            ctrlParams_t ctrlParams = {};
            if (readCtrlParams(root, &ctrlParams) == ESP_OK) {
                write_nvs(&ctrlParams);
                xQueueSend(ctrlParamsQueue, &ctrlParams, 50);
            } else {
                ESP_LOGW(tag, "Unable to read controller params. Controller not updated");
            }
        } else if (strncmp(subType, "ctrlSettings", 10) == 0) {
            ctrlSettings_t ctrlSettings = {};
            if (readCtrlSettings(root, &ctrlSettings) == ESP_OK) {
                xQueueSend(ctrlSettingsQueue, &ctrlSettings, 50);
            } else {
                ESP_LOGW(tag, "Unable to read controller settings. Controller not updated");
            }
        } else if (strncmp(subType, "ASSIGN", 6) == 0) {
            ESP_LOGI(tag, "Assigning temperature sensor");
            DS18B20_t sensor = {};
            if (readTempSensorParams(root, &sensor) == ESP_OK) {
                insertSavedRomCodes(sensor);
            } else {
                ESP_LOGW(tag, "Unable to assign temperature sensor");
            }
        }
    } else if (strncmp(type, "CMD", 3) == 0) {
        ESP_LOGI(tag, "Received CMD message\n");
        if (strncmp(subType, "OTA", 3) == 0) {
            ESP_LOGI(tag, "Received OTA request");
            ota_t ota;
            char* OTA_IP = cJSON_GetObjectItem(root, "IP")->valuestring;
            ota.len = strlen(OTA_IP);
            memcpy(ota.ip, OTA_IP, ota.len);
            ESP_LOGI(tag, "OTA IP set to %s", OTA_IP);
            xTaskCreate(&ota_update_task, "ota_update_task", 8192, (void*) &ota, 5, NULL);
        } else if (strncmp(subType, "ASSIGN", 6) == 0) {
            int start = cJSON_GetObjectItem(root, "start")->valueint;
            if (start) {
                ESP_LOGI(tag, "Received command to assign sensors");
                xTaskCreatePinnedToCore(&sensorAssignTask, "SensorAssign", 4096, (void*) ws, 5, &assignSensorHandle, 1);
            } else {
                ESP_LOGI(tag, "Deleting sensor assign task");
                vTaskDelete(assignSensorHandle);
                assignSensorHandle = NULL;
            } 
        } 
    } else {
        ESP_LOGW(tag, "Could not decode message with header: %s Subtype: %s", type, subType);
    }

    if (root != NULL) {
        cJSON_Delete(root);
    }
}

static void closeConnection(Websock *ws)
{
    ConnectionManager::removeConnection(ws);
    ConnectionManager::printConnections();
}

static void openConnection(Websock *ws) 
{
	ws->recvCb=myWebsocketRecv;
    ws->closeCb=closeConnection;
    ConnectionManager::addConnection(ws);
    ConnectionManager::printConnections();
    sendStates(ws);
    xTaskCreatePinnedToCore(&websocket_task, "webServer", 8192, ws, 2, NULL, 0);
}

static void sendStates(Websock* ws) 
{
    // Send initial states to client to configure settings
    cJSON *root;
	root = cJSON_CreateObject();

    ctrlSettings_t ctrlSettings = getControllerSettings();

    cJSON_AddStringToObject(root, "type", "status");
    cJSON_AddNumberToObject(root, "fanState", ctrlSettings.fanState);
    cJSON_AddNumberToObject(root, "flush", ctrlSettings.flush);
    cJSON_AddNumberToObject(root, "elementLow", ctrlSettings.elementLow);
    cJSON_AddNumberToObject(root, "elementHigh", ctrlSettings.elementHigh);
    cJSON_AddNumberToObject(root, "prodCondensor", ctrlSettings.prodCondensor);

    char* JSONptr = cJSON_Print(root);
    cJSON_Delete(root);

    if (wsSemaphore != NULL) {
        if(xSemaphoreTake(wsSemaphore, (TickType_t) 10) == pdTRUE) {
            if (ConnectionManager::checkConnection(ws) == ESP_OK) {
                cgiWebsocketSend(&httpdFreertosInstance.httpdInstance, ws, JSONptr, strlen(JSONptr), WEBSOCK_FLAG_NONE);
                xSemaphoreGive(wsSemaphore);
            } else {
                ESP_LOGI(tag, "Unable to access websocket shared resource to send states");
            } 
        }    
    }

    free(JSONptr);
}

static void sensorAssignTask(void *pvParameters)
{
    Websock* ws = (Websock*) pvParameters;
    int n_found = 0;

    while (true) {
        cJSON* root = cJSON_CreateObject();
        cJSON* sensors = cJSON_CreateArray();
        cJSON_AddStringToObject(root, "type", "sensorID");
        cJSON_AddItemToObject(root, "sensors", sensors);
        OneWireBus_ROMCode rom_codes[MAX_DEVICES] {}; 
        n_found = scanTempSensorNetwork(rom_codes);

        for (int i=0; i < n_found; i++) {
            cJSON* bytes = cJSON_CreateArray();
            for (int j=0; j < 8; j++) {
                cJSON* byte = cJSON_CreateNumber((int) rom_codes[i].bytes[j]);
                cJSON_AddItemToArray(bytes, byte);
            }
            cJSON_AddItemToArray(sensors, bytes);
        }
       
        char* JSONptr = cJSON_Print(root);
        cJSON_Delete(root);

        if (wsSemaphore != NULL) {
            if(xSemaphoreTake(wsSemaphore, (TickType_t) 10) == pdTRUE) {
                if (ConnectionManager::checkConnection(ws) == ESP_OK) {
                    cgiWebsocketSend(&httpdFreertosInstance.httpdInstance, ws, JSONptr, strlen(JSONptr), WEBSOCK_FLAG_NONE);
                    xSemaphoreGive(wsSemaphore);
                } else {
                    // Connection has been closed. Quit task
                    free(JSONptr);
                    vTaskDelete(NULL);
                }
            } else {
                ESP_LOGI(tag, "Unable to access websocket shared resource to send sensors");
            }
        }
        
        free(JSONptr);
        vTaskDelay(3000 / portTICK_PERIOD_MS);
    }
}

HttpdBuiltInUrl builtInUrls[]={
	ROUTE_REDIRECT("/", "index.html"),
    ROUTE_WS("/ws", openConnection),
    ROUTE_FILESYSTEM(),
	ROUTE_END()
};

void webServer_init(void)
{
    ESP_LOGI(tag, "Initializing webserver");
    EspFsConfig conf = {
		.memAddr = espfs_image_bin,
	};
    EspFs* fs = espFsInit(&conf);
    wsSemaphore = xSemaphoreCreateMutex();
    httpdRegisterEspfs(fs);
    esp_netif_init();
	httpdFreertosInit(&httpdFreertosInstance,
	                  builtInUrls,
	                  LISTEN_PORT,
	                  connectionMemory,
	                  ConnectionManager::MAX_CONNECTIONS,
	                  HTTPD_FLAG_NONE);
	httpdFreertosStart(&httpdFreertosInstance);
    ESP_LOGI(tag, "Webserver waiting for connections");
}

#ifdef __cplusplus
}
#endif