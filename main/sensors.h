#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "ds18b20.h"

// Expose queue handles for passing data between tasks
xQueueHandle tempQueue;
xQueueHandle hotSideTempQueue;
xQueueHandle coldSideTempQueue;
xQueueHandle flowRateQueue;

/*
*   --------------------------------------------------------------------  
*   temp_sensor_task
*   --------------------------------------------------------------------
*   Main temperature sensor task. Handles the initialization and reading
*   of the ds18b20 temperature sensors on the onewire bus
*/
void temp_sensor_task(void *pvParameters);

/*
*   --------------------------------------------------------------------  
*   sensor_init
*   --------------------------------------------------------------------
*   Initializes sensors on the one wire bus and the queues to pass data
*   between tasks
*/
esp_err_t sensor_init(uint8_t ds_pin, DS18B20_RESOLUTION res);

/*
*   --------------------------------------------------------------------  
*   flowmeter_task
*   --------------------------------------------------------------------
*   Main task responsible for reading the flowmeter
*/
void flowmeter_task(void *pvParameters);

/*
*   --------------------------------------------------------------------  
*   flowmeter_ISR
*   --------------------------------------------------------------------
*   Interrupt routine triggered by signals from the flowmeter
*/
void IRAM_ATTR flowmeter_ISR(void* arg);

/*
*   --------------------------------------------------------------------  
*   init_timer
*   --------------------------------------------------------------------
*   Initializes one of the ESP32 timers for measuring flowrate
*/
esp_err_t init_timer(void);

/*
*   --------------------------------------------------------------------  
*   readTemps
*   --------------------------------------------------------------------
*   Reads the temperatures from all connected sensors and loads them
*   into sensorTemps array
*/
void readTemps(float sensorTemps[]);

/*
*   --------------------------------------------------------------------  
*   checkPowerSupply
*   --------------------------------------------------------------------
*   Checks if DS18B20 sensors are connected in powered or parasitic
*   power mode
*/
void checkPowerSupply(void);


