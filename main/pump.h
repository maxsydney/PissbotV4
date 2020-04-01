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

        uint16_t getSpeed() const {return _pumpSpeed;}
        pumpMode_t getMode() const {return _mode;}
        bool isConfigured(void) const {return _isConfigured;}

    private:
        esp_err_t _initPump() const;

        bool _isConfigured = false;
        uint16_t _pumpSpeed = 0;;
        pumpMode_t _mode = pumpCtrl_active;
        ledc_channel_t _PWMChannel = LEDC_CHANNEL_0;
        int _pin = 0;
        ledc_timer_t _timerChannel = LEDC_TIMER_0;
};

#ifdef __cplusplus
}
#endif