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
#include <esp_event_loop.h>
#include <errno.h>
#include "freertos/queue.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "networking.h"
#include "messages.h"
#include "controller.h"
#include "main.h"
#include "sensors.h"
#include "gpio.h"
#include "driver/uart.h"

#define PORT_NUMBER 8001
#define BUFLEN 200
#define SSID "vodafoneB1100A_2GEXT"
#define PASSWORD "@leadership room 11"
#define MAX_MESSAGE_LEN 100
#define get_time_ms() (esp_timer_get_time() / 1000000.0)
#define ECHO_TEST_TXD  (GPIO_NUM_4)
#define ECHO_TEST_RXD  (GPIO_NUM_5)
#define ECHO_TEST_RTS  (UART_PIN_NO_CHANGE)
#define ECHO_TEST_CTS  (UART_PIN_NO_CHANGE)
#define BUF_SIZE (1024)

static void sendData(void* param);
static void recvData(void* param);
static bool socket_is_open(int socket);
static void write_nvs(Data* data);

static char tag[] = "socket server";

void wifi_connect(void)
{
    tcpip_adapter_init();
    esp_event_loop_init(event_handler, NULL);
    wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&config);
    esp_wifi_set_mode(WIFI_MODE_STA);

    wifi_config_t staConfig = {
        .sta = {
            .ssid="vodafoneB1100A_2GEXT",
            .password="@leadership room 11",
            .bssid_set=false
        }
    };
    esp_wifi_set_config(WIFI_IF_STA, &staConfig);
    esp_wifi_start();
    esp_wifi_connect();
}

void nvs_initialize(void)
{
    // Initialize NVS
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
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

esp_err_t event_handler(void *ctx, system_event_t *event)
{
    if (event->event_id == SYSTEM_EVENT_STA_GOT_IP) {
        printf("Our IP address is " IPSTR "\n", IP2STR(&event->event_info.got_ip.ip_info.ip));
        printf("We have now connected to a station and can do things...\n");
    } else if (event->event_id == SYSTEM_EVENT_STA_CONNECTED) {
        // Blink LED 
        flash_pin(LED_PIN, 100);
        ESP_LOGI(tag, "Connected to WiFi!\n");
    } else if (event->event_id ==SYSTEM_EVENT_STA_DISCONNECTED) {
         /* This is a workaround as ESP32 WiFi libs don't currently
           auto-reassociate. */
        flash_pin(LED_PIN, 100);
        ESP_LOGI(tag, "disconnect reason: %d", event->event_info.disconnected.reason);
        esp_err_t eet = esp_wifi_connect();
        ESP_LOGI(tag, "reconnected Ok or error: %d, authmode: %d", eet, event->event_info.connected.authmode);
    } else if (event->event_id == SYSTEM_EVENT_STA_DISCONNECTED) {
        ESP_LOGW(tag, "Could not connect!\n");
    }
    return ESP_OK;
}

void socket_server_task(void* params)
{
    struct sockaddr_in clientAddress;
    struct sockaddr_in serverAddress;

    // Create socket to listen on 
    int sock = socket(AF_INET,SOCK_STREAM, IPPROTO_TCP);
    if (sock < 0) {
        printf("Socket: %d %s", sock, strerror(errno));
        vTaskDelete(NULL);
    }

    // Bind server socket to a port 
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddress.sin_port = htons(PORT_NUMBER);
    int rc = bind(sock, (struct sockaddr*) &serverAddress, sizeof(serverAddress));
    if (rc < 0) {
        printf("Bind: %d %s", rc, strerror(errno));
        vTaskDelete(NULL);
    }

    // Flag socket as listening for new connections
    rc = listen(sock, 5);
    if (rc < 0) {
        printf("Listen: %d %s", rc, strerror(errno));
        vTaskDelete(NULL);
    }

    while(1) {
        // Listen for a new connection 
        ESP_LOGD(tag, "Waiting for new client connection\n");
        socklen_t clientAddressLength = sizeof(clientAddress);
        int clientSock = accept(sock, (struct sockaddr*) &clientAddress, &clientAddressLength);
        if (clientSock < 0) {
            printf("Accept: %d %s", clientSock, strerror(errno));
                vTaskDelete(NULL);
        }
        xTaskCreate(&sendData, "Data transmission", 2048, (void*) clientSock, 1, NULL);
        xTaskCreate(&recvData, "Receive data", 2048, (void*) clientSock, 2, NULL);
    }
}

static void write_nvs(Data* data)
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

static void recvData(void* param)
{
    int socket = (int) param;
    char recv_buf[BUFLEN] = {0};

    while(socket_is_open(socket)) {
        if (read(socket, recv_buf, BUFLEN-1)) {
            // Split into header and message
            char* header = strtok(recv_buf, "&");
            char* message = strtok(NULL, "&");

            if (strncmp(header, "CONN", 4) == 0) {              // Inital connection from client
                ESP_LOGI(tag, "Received new connection\n");
            } else if (strncmp(header, "INFO", 4) == 0) {       // New data packet received
                ESP_LOGI(tag, "Received INFO message\n");
                Data* data = decode_data(message);
                write_nvs(data);
                xQueueSend(dataQueue, data, 500);
                free(data);
            }
        }
    }
    vTaskDelete(NULL);
    return;
}

static void sendData(void* param)
{
    int socket = (int) param;
    float temp;
    float setpoint;
    float runtime;
    bool element_status;
    char message[MAX_MESSAGE_LEN];

    while(socket_is_open(socket)) {
        temp = get_temp();
        runtime = get_time_ms();
        setpoint = get_setpoint();
        element_status = get_element_status();
        Data settings = get_controller_settings();
        sprintf(message, "%.4f,%.4f,%.2f,%d,%f,%f,%f\n",temp, setpoint, runtime, element_status, settings.P_gain, settings.I_gain, settings.D_gain);
        send(socket, message, strlen(message), 0);
    }
    vTaskDelete(NULL);
    return;
}

static bool socket_is_open(int socket)
{
    static bool socket_active = false;
    if (send(socket, "BEAT\n", 5, 0) == -1) {       // Send heartbeat
        if (socket_active) {
            ESP_LOGI(tag, "%s\n", "Socket connection lost");
        }
        socket_active = false;
    } else {
        socket_active = true;
    }
    return socket_active;
}

void sendDataUART(void* param)
{
    float temp;
    float setpoint;
    float runtime;
    bool element_status;
    char message[MAX_MESSAGE_LEN] = {0};
    int len;

    while(true) {
        temp = get_temp();
        runtime = get_time_ms();
        setpoint = get_setpoint();
        element_status = get_element_status();
        Data settings = get_controller_settings();
        sprintf(message, "%.4f,%.4f,%.2f,%d,%f,%f,%f\n",temp, setpoint, runtime, element_status, settings.P_gain, settings.I_gain, settings.D_gain);
        len = uart_write_bytes(UART_NUM_1, (const char *) message, strlen(message));
        vTaskDelay(250 / portTICK_PERIOD_MS);
    }
}