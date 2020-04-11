#ifdef __cplusplus
extern "C" {
#endif

#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_netif.h"
#include "esp_log.h"
#include <libesphttpd/esp.h>
#include "libesphttpd/httpd.h"
#include "libesphttpd/cgiwifi.h"
#include "libesphttpd/cgiflash.h"
#include "libesphttpd/auth.h"
#include "libesphttpd/captdns.h"
#include "libesphttpd/httpd-espfs.h"
#include "espfs.h"
#include "espfs_image.h"
#include "libesphttpd/cgiwebsocket.h"
#include "libesphttpd/httpd-freertos.h"
#include "libesphttpd/route.h"
#include "cJSON.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "esp_spiffs.h"
#include "controlLoop.h"
#include "messages.h"
#include "networking.h"
#include "main.h"
#include "ota.h"
#include "webServer.h"
#include "sensors.h"

#define LED_PIN GPIO_NUM_2
#define GPIO_HIGH   1
#define GPIO_LOW    0
#define LISTEN_PORT     80u
#define MAX_CONNECTIONS 32u
#define STATIC_IP		"192.168.1.202"
#define SUBNET_MASK		"255.255.255.0"
#define GATE_WAY		"192.168.1.1"
#define DNS_SERVER		"8.8.8.8"

static char connectionMemory[sizeof(RtosConnType) * MAX_CONNECTIONS];
static const char *tag = "Webserver";
static HttpdFreertosInstance httpdFreertosInstance;
xTaskHandle socketSendHandle;

static bool checkWebsocketActive(volatile Websock* ws);
static void cleanJSONString(char* inputBuffer, char* destBuffer);
static void sendStates(Websock* ws);

void websocket_task(void *pvParameters) 
{
    Websock *ws = (Websock*) pvParameters;
    cJSON *root;
    float temps[n_tempSensors] = {0};
    float flowRate;
    char buff[512];
    Data ctrlSet;
    int64_t uptime_uS;

    while (true) {
        updateTemperatures(temps);
        root = cJSON_CreateObject();
        ctrlSet = get_controller_settings();
        uptime_uS = esp_timer_get_time() / 1000000;
        flowRate = get_flowRate();

        // Construct JSON object
        cJSON_AddStringToObject(root, "type", "data");
        cJSON_AddNumberToObject(root, "T_vapour", getTemperature(temps, T_refluxHot));
        cJSON_AddNumberToObject(root, "T_refluxInflow", getTemperature(temps, T_refluxCold));
        cJSON_AddNumberToObject(root, "T_productInflow", getTemperature(temps, T_productHot));
        cJSON_AddNumberToObject(root, "T_radiator", getTemperature(temps, T_productCold));
        cJSON_AddNumberToObject(root, "T_boiler", getTemperature(temps, T_boiler));
        cJSON_AddNumberToObject(root, "setpoint", ctrlSet.setpoint);
        cJSON_AddNumberToObject(root, "uptime", uptime_uS);
        cJSON_AddNumberToObject(root, "flowrate", flowRate);
        cJSON_AddNumberToObject(root, "P_gain", ctrlSet.P_gain);
        cJSON_AddNumberToObject(root, "I_gain", ctrlSet.I_gain);
        cJSON_AddNumberToObject(root, "D_gain", ctrlSet.D_gain);
        cJSON_AddNumberToObject(root, "boilerConc", getBoilerConcentration(getTemperature(temps, T_boiler)));
        cJSON_AddNumberToObject(root, "vapourConc", getVapourConcentration(getTemperature(temps, T_refluxHot)));
        char* JSONptr = cJSON_Print(root);
        strncpy(buff, JSONptr, 512);
        cJSON_Delete(root);
        free(JSONptr);      // Must free string pointer to avoid memory leak

        if ((!checkWebsocketActive(ws))) {
            ESP_LOGW(tag, "Deleting send task");
            vTaskDelete(NULL);
        } else {
            cgiWebsocketSend(&httpdFreertosInstance.httpdInstance,
                        ws, buff, strlen(buff), WEBSOCK_FLAG_NONE);
        }

        vTaskDelay(250 / portTICK_PERIOD_MS);
    }
}

static bool checkWebsocketActive(volatile Websock* ws)
{
    bool active = true;

    if (ws->conn == 0x0 || !wifiConnected) {
        ESP_LOGW(tag, "Websocket inactive. Closing connection");
        active = false;
    } else if (ws->conn->isConnectionClosed) {
        // This check is not redundant, if conn = 0x00 then this check causes panic handler to be invoked
        ESP_LOGW(tag, "Websocket inactive. Closing connection");
        active = false;
    }
    return active;
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
    char* arg = NULL;
    char msgBuffer[len] = {};
    char cleanJSON[len] = {};
    memcpy(msgBuffer, ++data, len-2);        // Drop leading "
    msgBuffer[len-2] = '\0';

    cleanJSONString(msgBuffer, cleanJSON);
    cJSON* root = cJSON_Parse(cleanJSON);

    if (root != NULL) {
        type = cJSON_GetObjectItem(root, "type")->valuestring;
        arg = cJSON_GetObjectItem(root, "arg")->valuestring;
    } else {
        ESP_LOGW(tag, "Unable to parse JSON string");
    }

    if (type != NULL) {
        ESP_LOGI(tag, "Type is: %s", type);
    }

    if (arg != NULL) {
        ESP_LOGI(tag, "Arg is: %s", arg);
    }

    if (strncmp(type, "INFO", 4) == 0) { 
        // Hand new data packet to controller      
        ESP_LOGI(tag, "Received INFO message\n");
        Data* data = decode_data(root);
        write_nvs(data);
        xQueueSend(dataQueue, data, 50);
        free(data);
    } else if (strncmp(type, "CMD", 3) == 0) {
        // Received new command
        ESP_LOGI(tag, "Received CMD message\n");
        if (strncmp(arg, "OTA", 16) == 0) {
            // We have received new OTA request. Run OTA
            ESP_LOGI(tag, "Received OTA request");
            // ota_t ota;
            // ota.len = strlen(cmd.arg);
            // memcpy(ota.ip, cmd.arg, ota.len);
            // ESP_LOGI(tag, "OTA IP set to %s", OTA_IP);
            // xTaskCreate(&ota_update_task, "ota_update_task", 8192, (void*) &ota, 5, NULL);
        } else if (strncmp(arg, "ASSIGN", 6) == 0) {
            ESP_LOGI(tag, "Received command to assign sensors");
        } else {
            // Command is for controller
            // xQueueSend(cmdQueue, &cmd, 50);
        }
    } else {
        ESP_LOGW(tag, "Could not decode message with header: %s Argument: %s", type, arg);
    }

    if (root != NULL) {
        cJSON_Delete(root);
    }
}

static void myWebsocketConnect(Websock *ws) 
{
	ws->recvCb=myWebsocketRecv;
    ESP_LOGI(tag, "Socket connected!!\n");
    sendStates(ws);
    xTaskCreatePinnedToCore(&websocket_task, "webServer", 8192, ws, 3, &socketSendHandle, 0);
}

static void sendStates(Websock* ws) 
{
    // Send initial states to client to configure settings
    cJSON *root;
	root = cJSON_CreateObject();
    char buff[128];

    bool fanState = get_fan_state();
    bool flush = getFlush();
    bool element1State = get_element1_status();
    bool element2State = get_element2_status();
    bool prodCondensorManual = get_productCondensorManual();

    cJSON_AddStringToObject(root, "type", "status");
    cJSON_AddNumberToObject(root, "fanState", fanState);
    cJSON_AddNumberToObject(root, "flush", flush);
    cJSON_AddNumberToObject(root, "element1State", element1State);
    cJSON_AddNumberToObject(root, "element2State", element2State);
    cJSON_AddNumberToObject(root, "prodCondensorManual", prodCondensorManual);

    printf("JSON state message\n");
    printf("%s\n", cJSON_Print(root));
    printf("JSON length: %d\n", strlen(cJSON_Print(root)));
    char* JSONptr = cJSON_Print(root);
    strcpy(buff, JSONptr);
    cJSON_Delete(root);
    free(JSONptr);
    cgiWebsocketSend(&httpdFreertosInstance.httpdInstance, ws, buff, strlen(buff), WEBSOCK_FLAG_NONE);
}

HttpdBuiltInUrl builtInUrls[]={
	ROUTE_REDIRECT("/", "index.html"),
    ROUTE_WS("/ws", myWebsocketConnect),
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
    httpdRegisterEspfs(fs);
    esp_netif_init();
	httpdFreertosInit(&httpdFreertosInstance,
	                  builtInUrls,
	                  LISTEN_PORT,
	                  connectionMemory,
	                  MAX_CONNECTIONS,
	                  HTTPD_FLAG_NONE);
	httpdFreertosStart(&httpdFreertosInstance);
    ESP_LOGI(tag, "Webserver waiting for connections");
}

#ifdef __cplusplus
}
#endif