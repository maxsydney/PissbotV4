#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#define LED_PIN 2
#define FLOW_METER_PIN 23
#define FAN_CTRL_PIN 25

xQueueHandle queue;