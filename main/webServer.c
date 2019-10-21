#ifdef __cplusplus
extern "C" {
#endif

#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "tcpip_adapter.h"
#include "esp_log.h"
#include <libesphttpd/esp.h>
#include "libesphttpd/httpd.h"
#include "libesphttpd/cgiwifi.h"
#include "libesphttpd/cgiflash.h"
#include "libesphttpd/auth.h"
#include "libesphttpd/captdns.h"
#include "libesphttpd/httpdespfs.h"
#include "libesphttpd/espfs.h"
#include "libesphttpd/webpages-espfs.h"
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


#define LED_PIN GPIO_NUM_2
#define GPIO_HIGH   1
#define GPIO_LOW    0
#define LISTEN_PORT     80u
#define MAX_CONNECTIONS 32u
#define STATIC_IP		"192.168.1.201"
#define SUBNET_MASK		"255.255.255.0"
#define GATE_WAY		"192.168.1.1"
#define DNS_SERVER		"8.8.8.8"

static char connectionMemory[sizeof(RtosConnType) * MAX_CONNECTIONS];

static const char *tag = "Webserver";
static HttpdFreertosInstance httpdFreertosInstance;

static bool checkWebsocketActive(Websock* ws);
static void sendStates(Websock* ws);

xTaskHandle socketSendHandle;

void websocket_task(void *pvParameters) 
{
    Websock *ws = (Websock*) pvParameters;
    float temps[n_tempSensors] = {0};
    float flowRate;
    char buff[3848];
    Data ctrlSet;
    int64_t uptime_uS;

    while (true) {
        getTemperatures(temps);
        ctrlSet = get_controller_settings();
        uptime_uS = esp_timer_get_time() / 1000000;
        flowRate = get_flowRate();
        snprintf(buff, 3848, "[%f, %f, %f, %f, %f, %f, %lld, %f, 0, %f, %f, %f, %f, %f]", temps[T_refluxHot], 
                                                                                   temps[T_refluxCold],
                                                                                   temps[T_productHot],
                                                                                   temps[T_productCold],
                                                                                   temps[T_boiler],
                                                                                   ctrlSet.setpoint, 
                                                                                   uptime_uS, 
                                                                                   flowRate,
                                                                                   ctrlSet.P_gain, 
                                                                                   ctrlSet.I_gain, 
                                                                                   ctrlSet.D_gain,
                                                                                   getBoilerConcentration(96),
                                                                                   getVapourConcentration(temps[T_refluxHot]+50));

        if ((!checkWebsocketActive(ws))) {
            ESP_LOGE(tag, "Deleting send task");
            vTaskDelete(NULL);
        } else {
            cgiWebsocketSend(&httpdFreertosInstance.httpdInstance,
                        ws, buff, strlen(buff), WEBSOCK_FLAG_NONE);
        }

        vTaskDelay(250 / portTICK_PERIOD_MS);
    }
}

static bool checkWebsocketActive(Websock* ws)
{
    bool active = true;

    if (ws->conn == 0x0) {
        ESP_LOGE(tag, "Closing connection");
        active = false;
    } else if (ws->conn->isConnectionClosed) {
        // This check is not redundant, if conn = 0x00 then this check causes panic handler to be invoked
        ESP_LOGE(tag, "Closing connection");
        active = false;
    }
    return active;
}

static void myWebsocketRecv(Websock *ws, char *data, int len, int flags) {
    char *msg = strtok(data, "\n");
    ESP_LOGI(tag, "Received msg: %s", msg);
	char* header = strtok(msg, "&");
    char* message = strtok(NULL, "&");

    if (strncmp(header, "INFO", 4) == 0) { 
        // Hand new data packet to controller      
        ESP_LOGI(tag, "Received INFO message\n");
        Data* data = decode_data(message);
        write_nvs(data);
        xQueueSend(dataQueue, data, 50);
        free(data);
    } else if (strncmp(header, "CMD", 3) == 0) {
        // Received new command
        ESP_LOGI(tag, "Received CMD message\n");
        Cmd_t cmd = decodeCommand(message);
        if (strncmp(cmd.cmd, "OTA", 16) == 0) {
            // We have received new OTA request. Run OTA
            printf("Received OTA message");
            ota_t ota;
            ota.len = strlen(cmd.arg);
            memcpy(ota.ip, cmd.arg, ota.len);
            printf("OTA IP set to %s\n", OTA_IP);
            xTaskCreate(&ota_update_task, "ota_update_task", 8192, (void*) &ota, 5, NULL);
        } else {
            // Command is for controller
            xQueueSend(cmdQueue, &cmd, 50);
        }
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

    bool fanState = get_fan_state();
    bool flush = getFlush();
    bool elementState = get_element_status();

    cJSON_AddNumberToObject(root, "fanState", fanState ? 1 : 0);
    cJSON_AddNumberToObject(root, "flush", flush  ? 1 : 0);
    cJSON_AddNumberToObject(root, "elementState", elementState  ? 1 : 0);

    printf("JSON state message\n");
    printf("%s\n", cJSON_Print(root));
}

HttpdBuiltInUrl builtInUrls[]={
	ROUTE_REDIRECT("/", "index.html"),
    ROUTE_WS("/ws", myWebsocketConnect),
    ROUTE_FILESYSTEM(),
	ROUTE_END()
};

void webServer_init(void)
{
    espFsInit((void*)(webpages_espfs_start));
	tcpip_adapter_init();
	httpdFreertosInit(&httpdFreertosInstance,
	                  builtInUrls,
	                  LISTEN_PORT,
	                  connectionMemory,
	                  MAX_CONNECTIONS,
	                  HTTPD_FLAG_NONE);
	httpdFreertosStart(&httpdFreertosInstance);
}

#ifdef __cplusplus
}
#endif
