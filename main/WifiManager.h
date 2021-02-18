#ifndef WIFI_H
#define WIFI_H

#include "PBCommon.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"

// This code is directly lifted from the esp idf wifi examples page. 
// TODO: Write a better Wifi wrapper 

// TODO: Move these within class scope
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

class WifiManager
{
    static constexpr const char* Name = "Wifi Manager";

    public:
        static PBRet connect(const char* ssid, const char* password);

    private:

        static EventGroupHandle_t _wifiEventGroup;

        // Event handler callback.
        static void _eventHandler(void* arg, esp_event_base_t eventBase, int32_t eventID, void* eventData);
};

#endif // WIFI_H