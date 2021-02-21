#ifndef SLOWPWM_H
#define SLOWPWM_H

#include "PBCommon.h"
#include "cJSON.h"

class SlowPWMConfig
{
    public:
        SlowPWMConfig(void) = default;
        explicit SlowPWMConfig(double PWMFreq)
            : PWMFreq(PWMFreq) {}

        double PWMFreq = 0.0;       // PWM frequency [Hz]

        static constexpr double MAX_PWM_FREQ = 60.0;
        static constexpr double MIN_PWM_FREQ = 0.5;
};

// Like PWM, but slower
class SlowPWM
{
    static constexpr const char *Name = "SlowPWM";

    public:
        SlowPWM(void) = default;
        explicit SlowPWM(const SlowPWMConfig& cfg);
        
        PBRet update(int64_t currTime);
        PBRet setDutyCycle(double dutyCycle);

        static PBRet checkInputs(const SlowPWMConfig& cfg);
        static PBRet loadFromJSON(SlowPWMConfig &cfg, const cJSON *cfgRoot);

        // Getters
        uint8_t getOutputState(void) const { return _outputState; }
        double getDutyCycle(void) const { return _dutyCycle; }
        bool isConfigured(void) const { return _configured; }

    private:

        PBRet _initFromParams(const SlowPWMConfig &cfg);

        SlowPWMConfig _cfg {};
        double _pwmDt = 0.0;
        double _dutyCycle = 0.0;
        int64_t _lastCycle = 0;
        uint8_t _outputState = 0;
        bool _configured = false;
};

#endif // SLOWPWM_H