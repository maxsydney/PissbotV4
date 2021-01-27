#ifndef PUMP_H
#define PUMP_H

#include "PBCommon.h"
#include "cJSON.h"
#include <driver/ledc.h>

enum class PumpMode {
    Off,
    ActiveControl,
    ManualControl
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
        explicit Pump(const PumpConfig& cfg);

        // Update
        PBRet updatePumpActiveControl(uint32_t pumpSpeed) { return _updatePump(pumpSpeed, PumpMode::ActiveControl); }
        PBRet updatePumpManualControl(uint32_t pumpSpeed)  { return _updatePump(pumpSpeed, PumpMode::ManualControl); }
        void setPumpMode(PumpMode pumpMode) { _pumpMode = pumpMode; }

        // Utility
        static PBRet checkInputs(const PumpConfig& cfg);
        static PBRet loadFromJSON(PumpConfig& cfg, const cJSON* cfgRoot);

        // Getters
        uint32_t getPumpSpeed(void) const;
        PumpMode getPumpMode(void) const { return _pumpMode; }
        bool isConfigured(void) const { return _configured; }

        static constexpr uint32_t PUMP_MIN_OUTPUT = 25;
        static constexpr uint32_t PUMP_MAX_OUTPUT = 512;
        static constexpr uint32_t FLUSH_SPEED = 256;

        // Friend class for unit testing
        friend class PumpUT;

    private:
        PBRet _initFromParams(const PumpConfig& cfg);

        // Update
        PBRet _updatePump(double pumpSpeed, PumpMode pumpMode);
        PBRet _drivePump(void) const;
        PBRet _setSpeed(int16_t pumpSpeed, PumpMode pumpMode);

        bool _configured = false;
        uint32_t _pumpSpeedActive = 0;
        uint32_t _pumpSpeedManual = 0;
        PumpMode _pumpMode = PumpMode::Off;
        PumpConfig _cfg;
};

#endif // PUMP_H