#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#define SW_VERSION 1.0      // Deprecated
#define CONTROL_LOOP_FREQUENCY 5

#define LED_PIN GPIO_NUM_2
#define LCD_ADDR 0x27
#define LCD_COLS 20
#define LCD_ROWS 4

typedef enum {
    T_refluxHot,
    T_boiler,
    T_productHot,
    T_productCold,
    T_refluxCold,
    n_tempSensors
} tempSensor;

extern xQueueHandle queue;