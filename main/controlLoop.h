#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#define CONTROL_LOOP_RATE 5.0f
#define CONTROL_LOOP_PERIOD 1.0f / CONTROL_LOOP_RATE
#define SENSOR_SAMPLE_RATE 5.0f
#define SENSOR_SAMPLE_PERIOD 1.0f / SENSOR_SAMPLE_RATE

extern xQueueHandle ctrlParamsQueue;
extern xQueueHandle ctrlSettingsQueue;

// Controller Settings
// Must remain in this header so other C files can include it
typedef struct { 
    float setpoint;
    float P_gain;
    float I_gain;
    float D_gain;
} ctrlParams_t;

typedef struct {
    int fanState;
    int flush;
    int elementLow;
    int elementHigh;
    int prodCondensor;
} ctrlSettings_t;

/*
*   --------------------------------------------------------------------  
*   updateTemperatures
*   --------------------------------------------------------------------
*   Retrieves the most recently read temperatures and writes them into
*   tempArray
*/
esp_err_t updateTemperatures(float tempArray[]);

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
bool get_element1_status(void);

bool get_element2_status(void);

bool get_productCondensorManual(void);

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
ctrlParams_t get_controller_params(void);

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

bool get_fan_state(void);

bool getFlush(void);

/*
*   --------------------------------------------------------------------  
*   setElementState
*   --------------------------------------------------------------------
*   Switches power to 2.4kW heating element
*/
void setElementState(int state);

/*
*   --------------------------------------------------------------------  
*   computeVapourPressure
*   --------------------------------------------------------------------
*   Computes the partial vapour pressure of a gas in kPa based on Antoine 
*   equation constants.
*/
float computeVapourPressure(float A, float B, float C, float T);

/*
*   --------------------------------------------------------------------  
*   computeLiquidEthConcentration
*   --------------------------------------------------------------------
*   Computes the mol fraction of ethanol in a boiling mash
*/
float computeLiquidEthConcentration(float temp);

/*
*   --------------------------------------------------------------------  
*   computeVapourEthConcentration
*   --------------------------------------------------------------------
*   Computes the mol fraction of ethanol in ethanol vapour
*/
float computeVapourEthConcentration(float temp);

float getBoilerConcentration(float boilerTemp);

float getVapourConcentration(float vapourTemp);

ctrlParams_t getSettingsFromNVM(void);

#ifdef __cplusplus
}
#endif
