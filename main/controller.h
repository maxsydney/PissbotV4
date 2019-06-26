#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#define CONTROL_LOOP_RATE 5.0f
#define CONTROL_LOOP_PERIOD 1.0f / CONTROL_LOOP_RATE
#define SENSOR_SAMPLE_RATE 5.0f
#define SENSOR_SAMPLE_PERIOD 1.0f / SENSOR_SAMPLE_RATE

xQueueHandle dataQueue;
uint16_t ctrl_loop_period_ms;;

typedef struct { 
    float setpoint;
    float P_gain;
    float I_gain;
    float D_gain;
} Data;

/*
*   --------------------------------------------------------------------  
*   getTemperatures
*   --------------------------------------------------------------------
*   Retrieves the most recently read temperatures and writes them into
*   tempArray
*/
esp_err_t getTemperatures(float tempArray[]);

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

/*
*   --------------------------------------------------------------------  
*   setFlush
*   --------------------------------------------------------------------
*   Sets the pump to a fixed speed to flush all air out of the system
*/
void setFlush(bool state);

/*
*   --------------------------------------------------------------------  
*   checkFan
*   --------------------------------------------------------------------
*   Enables automatic handling of radiator fan. Fan will automatically switch
*   on when the hot side temperature is above a threshold, and switched off
*   when the system has cooled below the threshold 
*/
void checkFan(double T1);


