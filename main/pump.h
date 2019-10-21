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
        Pump(gpio_num_t pin, ledc_channel_t PWMChannel, ledc_timer_t timerChannel);
        Pump() {};

        void commandPump();

        void setSpeed(int16_t speed);
        void setMode(pumpMode_t mode) {_mode = mode;};

        uint16_t getSpeed() const {return _pumpSpeed;};
        pumpMode_t getMode() const {return _mode;};

    private:
        void _initPump() const;

        uint16_t _pumpSpeed;
        pumpMode_t _mode;
        ledc_channel_t _PWMChannel;
        int _pin;
        ledc_timer_t _timerChannel;
};

#ifdef __cplusplus
}
#endif