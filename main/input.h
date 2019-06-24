#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

typedef enum {
    input_up,
    input_down,
    input_left,
    input_mid,
    input_none
} inputType;

typedef struct {
    inputType button;
    int64_t time;
} buttonPress;

xQueueHandle inputQueue;

esp_err_t init_input(void);

bool debounceInput(buttonPress btnEvent);

void IRAM_ATTR input_ISR_handler(void* arg);

void input_task(void *pvParameters);