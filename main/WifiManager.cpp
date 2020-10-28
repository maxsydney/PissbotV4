#include "WifiManager.h"
#include <cstring>
#include <string>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"


int WifiManager::_numRetries = 0;
EventGroupHandle_t WifiManager::_wifiEventGroup = nullptr;

void WifiManager::_eventHandler(void* arg, esp_event_base_t eventBase, int32_t eventID, void* eventData)
{
    if (eventBase == WIFI_EVENT) {
        switch (eventID)
        {
            // Station is initialized. Attempt to connect to network
            case WIFI_EVENT_STA_START:
            {
                esp_wifi_connect();
                break;
            }
            // Lost connection to network. Attempt to reconnect
            case WIFI_EVENT_STA_DISCONNECTED:
            {
                ESP_LOGW(WifiManager::Name, "Failed to connect to Wifi network");
                if (_numRetries < MAX_RETRIES) {
                    esp_wifi_connect();
                    _numRetries++;
                    ESP_LOGI(WifiManager::Name, "Attempting to reconnect");
                } else {
                    xEventGroupSetBits(_wifiEventGroup, WIFI_FAIL_BIT);
                    ESP_LOGE(WifiManager::Name,"The maximum number of retry attempts was reached");
                }
                break;
            }
            default:
            {
                ESP_LOGI(WifiManager::Name, "Unknown Wifi event received (%d)", eventID);
            }
        }
    } else if (eventBase == IP_EVENT && eventID == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) eventData;
        ESP_LOGI(WifiManager::Name, "Got IP:" IPSTR, IP2STR(&event->ip_info.ip));
        _numRetries = 0;
        xEventGroupSetBits(_wifiEventGroup, WIFI_CONNECTED_BIT);
    }   
}

PBRet WifiManager::connect(const char* ssid, const char* password)
{
    _wifiEventGroup = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // TODO: Error checking here
    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &WifiManager::_eventHandler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &WifiManager::_eventHandler, NULL);

    wifi_config_t wifiConfig;
    wifiConfig.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    wifiConfig.sta.pmf_cfg.capable = true;
    wifiConfig.sta.pmf_cfg.required = false;
    strncpy((char*) wifiConfig.sta.ssid, ssid, 32);
    strncpy((char*) wifiConfig.sta.password, password, 64);

    ESP_LOGW(WifiManager::Name, "SSID: %s", wifiConfig.sta.ssid);
    ESP_LOGW(WifiManager::Name, "Password: %s", wifiConfig.sta.password);

    // TODO: Add proper error checking on these function calls
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifiConfig) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(WifiManager::Name, "Wifi initialized as station");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(_wifiEventGroup,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(WifiManager::Name, "Connected to ap %s", wifiConfig.sta.ssid);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(WifiManager::Name, "Failed to connect to %s", wifiConfig.sta.ssid);
    } else {
        ESP_LOGE(WifiManager::Name, "UNEXPECTED EVENT");
    }

    /* The event will not be processed after unregister */
    ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &WifiManager::_eventHandler));
    ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &WifiManager::_eventHandler));
    vEventGroupDelete(_wifiEventGroup);

    return PBRet::SUCCESS;
}