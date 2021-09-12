#ifndef PUMP_H
#define PUMP_H

#include "PBCommon.h"
#include "cJSON.h"
#include <driver/ledc.h>

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
        explicit Pump(const PumpConfig& cfg);

        // Update
        PBRet updatePumpSpeed(uint32_t pumpSpeed);      // TODO: make this function body _updatePump

        // Utility
        static PBRet checkInputs(const PumpConfig& cfg);
        static PBRet loadFromJSON(PumpConfig& cfg, const cJSON* cfgRoot);

        // Getters
        uint32_t getPumpSpeed(void) const;
        bool isConfigured(void) const { return _configured; }

        static constexpr uint32_t PUMP_OFF = 0;
        static constexpr uint32_t PUMP_IDLE_SPEED = 50;
        static constexpr uint32_t PUMP_MAX_SPEED = 1024;        // Based on 9 bit PWM
        static constexpr uint32_t FLUSH_SPEED = 1024;

        // Friend class for unit testing
        friend class PumpUT;

    private:
        PBRet _initFromParams(const PumpConfig& cfg);

        // Update
        PBRet _drivePump(uint32_t pumpSpeed) const;

        bool _configured = false;
        uint32_t _pumpSpeed = 0;
        PumpConfig _cfg;
};

#endif // PUMP_H