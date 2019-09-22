#pragma once

typedef enum {
    pumpCtrl_active,
    pumpCtrl_fixed
} pumpMode_t;

typedef struct {
    uint32_t pumpSpeed;
    pumpMode_t mode;
} pumpCtrl_t;

/*
*   --------------------------------------------------------------------  
*   initPumps
*   --------------------------------------------------------------------
*   Initializes the PWM pins used to send the controller output signal to 
*   the pump control circuits.
*/
void initPumps();

/*
*   --------------------------------------------------------------------  
*   set_motor_speed
*   --------------------------------------------------------------------
*   Updates the contol signal sent to the pump
*/
void set_motor_speed(int speed);