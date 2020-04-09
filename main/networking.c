#include <stdio.h>
#include <esp_log.h>
#include <tcpip_adapter.h>
#include <esp_wifi.h>
#include <lwip/sockets.h>
#include <tcpip_adapter.h>
#include <esp_wifi.h>
#include <esp_err.h>
#include <esp_timer.h>
#include <esp_event.h>
#include <string.h>
#include <errno.h>
#include "freertos/queue.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "networking.h"
#include "pinDefs.h"
#include "messages.h"
#include "controlLoop.h"
#include "main.h"
#include "sensors.h"
#include "gpio.h"
#include "driver/uart.h"

#define PORT_NUMBER 8001
#define BUFLEN 200
#define SSID "vodafoneB1100A_2GEXT"
#define PASSWORD "@leadership room 11"
#define ECHO_TEST_TXD  (GPIO_NUM_14)
#define ECHO_TEST_RXD  (GPIO_NUM_13)
#define ECHO_TEST_RTS  (UART_PIN_NO_CHANGE)
#define ECHO_TEST_CTS  (UART_PIN_NO_CHANGE)
#define BUF_SIZE (1024)
#define STATIC_IP		"192.168.1.202"
#define SUBNET_MASK		"255.255.255.0"
#define GATE_WAY		"192.168.1.1"
#define DNS_SERVER		"8.8.8.8"

static char tag[] = "socket server";
bool wifiConnected = false;

void wifi_connect(void)
{
    tcpip_adapter_ip_info_t ipInfo;
    wifi_config_t staConfig = {
        .sta = {
            // .ssid="vodafoneB1100A_2GEXT",
            .ssid="vodafoneB1100A",
            .password="@leadership room 11",
            .bssid_set=false
        }
    };
    tcpip_adapter_init();

    //For using of static IP
	tcpip_adapter_dhcpc_stop(TCPIP_ADAPTER_IF_STA); // Don't run a DHCP client

	//Set static IP
	inet_pton(AF_INET, STATIC_IP, &ipInfo.ip);
	inet_pton(AF_INET, GATE_WAY, &ipInfo.gw);
	inet_pton(AF_INET, SUBNET_MASK, &ipInfo.netmask);
	tcpip_adapter_set_ip_info(TCPIP_ADAPTER_IF_STA, &ipInfo);


    esp_event_loop_init(WiFi_event_handler, NULL);

    wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&config);
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &staConfig);
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_connect());
}

void uart_initialize(void)
{
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    uart_param_config(UART_NUM_1, &uart_config);
    uart_set_pin(UART_NUM_1, ECHO_TEST_TXD, ECHO_TEST_RXD, ECHO_TEST_RTS, ECHO_TEST_CTS);
    uart_driver_install(UART_NUM_1, BUF_SIZE * 2, 0, 0, NULL, 0);
    ESP_LOGI(tag, "UART Initialized");
}

esp_err_t WiFi_event_handler(void *ctx, system_event_t *event)
{
    if (event->event_id == SYSTEM_EVENT_STA_GOT_IP) {
        ESP_LOGI(tag, "Our IP address is " IPSTR, IP2STR(&event->event_info.got_ip.ip_info.ip));
        ESP_LOGI(tag, "We have now connected to a station and can serve the dashboard");
    } else if (event->event_id == SYSTEM_EVENT_STA_CONNECTED) {
        // Blink LED 
        flash_pin(LED_PIN, 100);
        ESP_LOGI(tag, "Connected to WiFi!");
        wifiConnected = true;
    } else if (event->event_id ==SYSTEM_EVENT_STA_DISCONNECTED) {
        // This is a workaround as ESP32 WiFi libs don't currently auto-reassociate.
        flash_pin(LED_PIN, 100);
        ESP_LOGI(tag, "disconnect reason: %d", event->event_info.disconnected.reason);
        wifiConnected = false;
        esp_err_t eet = esp_wifi_connect();
        ESP_LOGI(tag, "reconnected Ok or error: %d, authmode: %d", eet, event->event_info.connected.authmode);
    }

    return ESP_OK;
}

void write_nvs(Data* data)
{
    nvs_handle nvs;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &nvs);
    if (err != ESP_OK) {
        ESP_LOGI(tag, "Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    } else {
        ESP_LOGI(tag, "NVS handle opened successfully\n");
    }
 
    nvs_set_i32(nvs, "setpoint", (int32_t)(data->setpoint * 1000));
    nvs_set_i32(nvs, "P_gain", (int32_t)(data->P_gain * 1000));
    nvs_set_i32(nvs, "I_gain", (int32_t)(data->I_gain * 1000));
    nvs_set_i32(nvs, "D_gain", (int32_t)(data->D_gain * 1000));
    ESP_ERROR_CHECK(nvs_commit(nvs));
    nvs_close(nvs);
}

void heartBeatTask(void* param)
{
    // Provide visual indication that things are still running
    portTickType xLastWakeTime = xTaskGetTickCount();

    // TODO: Get state and change LED patter/message based on state

    while (true) {
        flash_pin(LED_PIN, 100);
        ESP_LOGI("Pissbot", "HEARTBEAT");
        vTaskDelayUntil(&xLastWakeTime, 5000 / portTICK_PERIOD_MS);
    }
}