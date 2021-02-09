#ifndef FLOWMETER_H
#define FLOWMETER_H

#include "PBCommon.h"
#include "cJSON.h"

struct FlowmeterConfig
{
    gpio_num_t flowmeterPin = (gpio_num_t)GPIO_NUM_NC;
    double kFactor = 0.0;       // Assume linear for now. Add several k factors if required
};

class Flowmeter
{
    static constexpr const char* Name = "Flowmeter";

    public:
        // Constructors
        Flowmeter(void) = default;
        explicit Flowmeter(const FlowmeterConfig& cfg);

        // Update
        PBRet readMassFlowrate(int64_t t, double& flowrate);

        // Utility
        static PBRet checkInputs(const FlowmeterConfig& cfg);
        static PBRet loadFromJSON(FlowmeterConfig& cfg, const cJSON* cfgRoot);

        // Getters
        double getFlowrate(void) const { return _flowrate; }
        bool isConfigured(void) const { return _configured; }

        // Increment frequency counter
        void incrementFreqCounter(void) volatile;

        // Flowmeter ISR
        static void IRAM_ATTR flowmeterISR(void* arg);

        friend class FlowmeterUT;

    private:
        PBRet _initFromParams(const FlowmeterConfig& cfg);
        FlowmeterConfig _cfg {};
        int64_t _lastUpdateTime = 0;        // Time of last update [uS]
        uint16_t _freqCounter = 0;          // Internal pulse counter
        double _flowrate = 0;               // Current flowrate [kg/s]
        bool _configured = false;
};

#endif // FLOWMETER_H