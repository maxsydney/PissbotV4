#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#define CONTROL_LOOP_RATE 5.0f
#define CONTROL_LOOP_PERIOD 1.0f / CONTROL_LOOP_RATE
#define SENSOR_SAMPLE_RATE 5.0f
#define SENSOR_SAMPLE_PERIOD 1.0f / SENSOR_SAMPLE_RATE

xQueueHandle dataQueue;

typedef struct { 
    float setpoint;
    float P_gain;
    float I_gain;
    float D_gain;
} Data;

/*
*   --------------------------------------------------------------------  
*   get_hot_temp
*   --------------------------------------------------------------------
*   Returns the most recent temperature reasing from the distiller
*   outflow temperature probe 
*/
float get_hot_temp(void);

/*
*   --------------------------------------------------------------------  
*   get_cold_temp
*   --------------------------------------------------------------------
*   Returns the most recent temperature reasing from the radiator
*   outflow temperature probe 
*/
float get_cold_temp(void);

/*
*   --------------------------------------------------------------------  
*   get_setpoint
*   --------------------------------------------------------------------
*   Returns the current controller setpoint
*/
float get_setpoint(void);

/*
*   --------------------------------------------------------------------  
*   get_flowRate
*   --------------------------------------------------------------------
*   Returns the most recent flowrate measurement in L/min. Does not work
*   accurately at very low flow rates
*/
float get_flowRate(void);

/*
*   --------------------------------------------------------------------  
*   get_element_status
*   --------------------------------------------------------------------
*   Not used on current PCB revision
*/
bool get_element_status(void);

/*
*   --------------------------------------------------------------------  
*   controller_init
*   --------------------------------------------------------------------
*   Initializes the controller with parameters from NVS
*/
esp_err_t controller_init(uint8_t frequency);

/*
*   --------------------------------------------------------------------  
*   nvs_initialize
*   --------------------------------------------------------------------
*   Initializes the NVS memory driver to access stored controller settings
*/
void nvs_initialize(void);

/*
*   --------------------------------------------------------------------  
*   control_loop
*   --------------------------------------------------------------------
*   Main temperature control task. Implements a simple PID controller 
*   with an anti integral windup strategy. Controller parameters are
*   configurable from the python GUI or the web interface
*/
void control_loop(void* params);

/*
*   --------------------------------------------------------------------  
*   get_controller_settings
*   --------------------------------------------------------------------
*   Returns a struct containing all controller parameters
*/
Data get_controller_settings(void);

/*
*   --------------------------------------------------------------------  
*   setFanState
*   --------------------------------------------------------------------
*   Switches the radiator fan on and off. Not currently implemented but
*   will be available in coming PCB revisions
*/
void setFanState(int state);

void setFlush(bool state);


