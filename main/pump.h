#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <driver/ledc.h>

constexpr uint16_t PUMP_MIN_OUTPUT = 25;
constexpr uint16_t PUMP_MAX_OUTPUT = 512;
constexpr uint16_t FLUSH_SPEED = 512;

enum class PumpMode {
    ACTIVE,
    FIXED
};

class Pump
{
    public:
        Pump(gpio_num_t pin, ledc_channel_t PWMChannel, ledc_timer_t timerChannel);
        Pump() {};

        void commandPump();

        void setSpeed(int16_t speed);
        void setMode(PumpMode pumpMode) {_pumpMode = pumpMode;};

        uint16_t getSpeed() const {return _pumpSpeed;}
        const PumpMode& getMode() const {return _pumpMode;}
        bool isConfigured(void) const {return _configured;}

    private:
        esp_err_t _initPump() const;

        bool _configured = false;
        uint16_t _pumpSpeed = 0;;
        PumpMode _pumpMode = PumpMode::ACTIVE;
        ledc_channel_t _PWMChannel = LEDC_CHANNEL_0;
        int _pin = 0;
        ledc_timer_t _timerChannel = LEDC_TIMER_0;
};

#ifdef __cplusplus
}
#endif