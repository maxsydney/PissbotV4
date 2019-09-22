#pragma once
#include "esp_event.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "controlLoop.h"

#define MAX_MESSAGE_LEN 100
#define get_time_ms() (esp_timer_get_time() / 1000000.0)

/*
*   --------------------------------------------------------------------  
*   wifi_connect
*   --------------------------------------------------------------------
*   Initializes the WiFi driver and connects to the local network. Since
*   new web interface WiFi is no longer needed on the ESP32. This will
*   eventually be deprecated.
*/
void wifi_connect(void);

/*
*   --------------------------------------------------------------------  
*   WiFi_event_handler
*   --------------------------------------------------------------------
*   Handles all WiFi events. Events handled are:
*       GOT IP: Reports IP address on local network
*       CONNECTED: WiFi driver is available for use
*       DISCONNECTED: Will try to reconnect to WiFi
*/
esp_err_t WiFi_event_handler(void *ctx, system_event_t *event);

/*
*   --------------------------------------------------------------------  
*   uart_initialize
*   --------------------------------------------------------------------
*   Initializes the UART for bi-directional communications with raspberry
*   pi webserver. Deprecated in current version, might be re-used in the 
*   future for serial communications
*/
void uart_initialize(void);

/*
*   --------------------------------------------------------------------  
*   uart_initialize
*   --------------------------------------------------------------------
*   Initializes the UART for bi-directional communications with raspberry
*   pi webserver
*/
void write_nvs(Data* data);