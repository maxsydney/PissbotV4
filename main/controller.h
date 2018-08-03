#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#define CONTROL_LOOP_RATE 5.0f
#define CONTROL_LOOP_PERIOD 1.0f / CONTROL_LOOP_RATE
#define SENSOR_SAMPLE_RATE 5.0f
#define SENSOR_SAMPLE_PERIOD 1.0f / SENSOR_SAMPLE_RATE

typedef struct { 
    float setpoint;
    float P_gain;
    float I_gain;
    float D_gain;
} Data;

float get_hot_temp(void);
float get_cold_temp(void);
float get_setpoint(void);
float get_flowRate(void);
bool get_element_status(void);
esp_err_t controller_init(uint8_t frequency);
void control_loop(void* params);
Data get_controller_settings(void);

xQueueHandle dataQueue;