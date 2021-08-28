#include "WifiManager.h"
#include <cstring>
#include <string>
#include <lwip/sockets.h>
#include <tcpip_adapter.h>
#include <esp_wifi.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"

#include "lwip/err.h"
#include "lwip/sys.h"

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
                // Keep spamming retry attempts until we succeeds
                ESP_LOGW(WifiManager::Name, "Failed to connect to Wifi network");
                esp_wifi_connect();
                ESP_LOGI(WifiManager::Name, "Attempting to reconnect");
                break;
            }
            case WIFI_EVENT_STA_CONNECTED:
            {
                ESP_LOGI(WifiManager::Name, "Successfully connected to WiFi network!");
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
        xEventGroupSetBits(_wifiEventGroup, WIFI_CONNECTED_BIT);
    }   
}

PBRet WifiManager::connect(const char* ssid, const char* password)
{
    _wifiEventGroup = xEventGroupCreate();

    if (esp_netif_init() != ESP_OK) {
        ESP_LOGW(WifiManager::Name, "Failed to initialize netif interface");
        return PBRet::FAILURE;
    }

    if (esp_event_loop_create_default() != ESP_OK) {
        ESP_LOGW(WifiManager::Name, "Failed to create WiFi event loop");
        return PBRet::FAILURE;
    }

    esp_netif_t* netif = esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    if (esp_wifi_init(&cfg) != ESP_OK) {
        ESP_LOGW(WifiManager::Name, "Failed to initialize WiFi driver");
        return PBRet::FAILURE;
    }

    
    if (esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &WifiManager::_eventHandler, NULL) != ESP_OK) {
        ESP_LOGW(WifiManager::Name, "Failed to register WiFi event handler");
        return PBRet::FAILURE;
    }

    if (esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &WifiManager::_eventHandler, NULL) != ESP_OK) {
        ESP_LOGW(WifiManager::Name, "Failed to register IP event handler");
        return PBRet::FAILURE;
    }

    // Set static IP
    // TODO: Implement better way to configure stating IP (from config?)
    esp_netif_dhcpc_stop(netif);
    esp_netif_ip_info_t ip_info;
    IP4_ADDR(&ip_info.ip, 192, 168, 1, 201);
   	IP4_ADDR(&ip_info.gw, 192, 168, 15, 1);
   	IP4_ADDR(&ip_info.netmask, 255, 255, 255, 0);
    if (esp_netif_set_ip_info(netif, &ip_info) != ESP_OK) {
        ESP_LOGW(WifiManager::Name, "Failed to register static IP");
        return PBRet::FAILURE;
    }

    wifi_config_t wifiConfig {};
    wifiConfig.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    wifiConfig.sta.pmf_cfg.capable = true;
    wifiConfig.sta.pmf_cfg.required = false;
    strncpy((char*) wifiConfig.sta.ssid, ssid, 31);
    strncpy((char*) wifiConfig.sta.password, password, 63);

    esp_err_t err = ESP_OK;
    err |= esp_wifi_set_mode(WIFI_MODE_STA);
    err |= esp_wifi_set_config(WIFI_IF_STA, &wifiConfig);
    err |= esp_wifi_start();
    if (err != ESP_OK) {
        ESP_LOGW(WifiManager::Name, "Failed to configure WiFi driver");
        return PBRet::FAILURE;
    }

    ESP_LOGI(WifiManager::Name, "Wifi initialized as station");
    return PBRet::SUCCESS;
}