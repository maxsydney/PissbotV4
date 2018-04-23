#pragma once
#include "esp_event.h"
#include "nvs_flash.h"
#include "nvs.h"

void wifi_connect(void);
esp_err_t event_handler(void *ctx, system_event_t *event);
void socket_server_task(void* params);
void nvs_initialize(void);

nvs_handle nvs;

