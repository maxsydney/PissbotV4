#pragma once

#include <driver/ledc.h>

enum class PumpMode {
    ACTIVE,
    FIXED
};

class PumpCfg
{
    public:
        PumpCfg() = default;
        PumpCfg(gpio_num_t pin, ledc_channel_t PWMChannel, ledc_timer_t timerChannel);

        bool isConfigured(void) const { return _configured; }
        gpio_num_t pin = GPIO_NUM_0;
        ledc_channel_t PWMChannel = LEDC_CHANNEL_0;
        ledc_timer_t timerChannel = LEDC_TIMER_0;
    
    private:
        bool _configured = false;
};

class Pump
{
    public:
        Pump() = default;
        Pump(const PumpCfg& cfg);

        void commandPump();
        void setSpeed(int16_t speed);
        void setMode(PumpMode pumpMode) {_pumpMode = pumpMode;};

        uint16_t getSpeed() const {return _pumpSpeed;}
        const PumpMode& getMode() const {return _pumpMode;}
        bool isConfigured(void) const {return _configured;}

        static constexpr uint16_t PUMP_MIN_OUTPUT = 25;
        static constexpr uint16_t PUMP_MAX_OUTPUT = 512;
        static constexpr uint16_t FLUSH_SPEED = 256;

    private:
        esp_err_t _initPump() const;

        bool _configured = false;
        uint16_t _pumpSpeed = 0;;
        PumpMode _pumpMode = PumpMode::ACTIVE;
        PumpCfg _cfg;
};