#pragma once

#include "PBCommon.h"
#include "cJSON.h"
#include <driver/ledc.h>

enum class PumpMode {
    ACTIVE,
    FIXED
};

class PumpConfig
{
    public:
        PumpConfig() = default;
        PumpConfig(gpio_num_t pumpGPIO, ledc_channel_t PWMChannel, ledc_timer_t timerChannel)
            : pumpGPIO(pumpGPIO), PWMChannel(PWMChannel), timerChannel(timerChannel) {}

        gpio_num_t pumpGPIO = (gpio_num_t) GPIO_NUM_NC;
        ledc_channel_t PWMChannel = LEDC_CHANNEL_0;
        ledc_timer_t timerChannel = LEDC_TIMER_0;
};

class Pump
{
    static constexpr const char* Name = "Pump";

    public:
        // Constructors
        Pump() = default;
        Pump(const PumpConfig& cfg);

        // Update
        void commandPump();
        void setSpeed(int16_t speed);
        void setMode(PumpMode pumpMode) {_pumpMode = pumpMode;};

        // Utility
        static PBRet checkInputs(const PumpConfig& cfg);
        static PBRet loadFromJSON(PumpConfig& cfg, const cJSON* cfgRoot);

        // Getters
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
        PumpConfig _cfg;
};