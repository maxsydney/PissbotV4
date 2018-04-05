#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

float get_temp(void);
float get_setpoint(void);
bool get_element_status(void);
esp_err_t controller_init(uint8_t frequency);
void control_loop(void* params);

typedef struct { 
    float setpoint;
    uint8_t P_gain;
    uint8_t I_gain;
    uint8_t D_gain;
} Data;

xQueueHandle dataQueue;