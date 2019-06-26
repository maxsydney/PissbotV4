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

/*
*   --------------------------------------------------------------------  
*   init_input
*   --------------------------------------------------------------------
*   Initializes the queue for passing input messages to menu task
*/
esp_err_t init_input(void);

/*
*   --------------------------------------------------------------------  
*   debounceInput
*   --------------------------------------------------------------------
*   Debounces the input signals coming from control
*/
bool debounceInput(buttonPress btnEvent);

/*
*   --------------------------------------------------------------------  
*   input_ISR_handler
*   --------------------------------------------------------------------
*   Handles all incoming signals on input pins and passes them to the
*   input queue
*/
void IRAM_ATTR input_ISR_handler(void* arg);