#pragma once

/*
*   --------------------------------------------------------------------  
*   pwm_init
*   --------------------------------------------------------------------
*   Initializes the PWM pin used to send the controller output signal to 
*   the pump control circuit.
*/
void pwm_init();

/*
*   --------------------------------------------------------------------  
*   set_motor_speed
*   --------------------------------------------------------------------
*   Updates the contol signal sent to the pump
*/
void set_motor_speed(int speed);