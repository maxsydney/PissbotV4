#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#define SW_VERSION 1.0

#define LED_PIN 2
#define LCD_ADDR 0x27
#define LCD_COLS 20
#define LCD_ROWS 4

typedef enum {
    T_refluxHot,
    T_refluxCold,
    T_productHot,
    T_productCold,
    T_boiler
} tempSensor;

xQueueHandle queue;