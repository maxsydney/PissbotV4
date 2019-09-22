#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <driver/ledc.h>

typedef enum {
    pumpCtrl_active,
    pumpCtrl_fixed
} pumpMode_t;

class Pump
{
    public:
        Pump(int pin, ledc_channel_t PWMChannel);
        Pump();

        void commandPump() const;

        void setSpeed(uint16_t speed) {_pumpSpeed = speed;};
        void setMode(pumpMode_t mode) {_mode = mode;};

        uint16_t getSpeed() const {return _pumpSpeed;};
        pumpMode_t getMode() const {return _mode;};

    private:
        void _initPump() const;

        uint16_t _pumpSpeed;
        pumpMode_t _mode;
        ledc_channel_t _PWMChannel;
        int _pin;

};

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

#ifdef __cplusplus
}
#endif