#ifndef PB_DS18B20_H
#define PB_DS18B20_H

#include "PBCommon.h"
#include "ds18b20.h"
#include "cJSON.h"
#include "Generated/DS18B20Messaging.h"
#include "Generated/SensorManagerMessaging.h"

constexpr uint8_t ROM_SIZE = 8;
using PBDS18B20Sensor = DS18B20Sensor<ROM_SIZE>;
using PBAssignSensorCommand = AssignSensorCommand<ROM_SIZE>;

class Ds18b20Config
{
    public:
        Ds18b20Config(void) = default;
        Ds18b20Config(const OneWireBus_ROMCode& romCode, double calibLinear, 
                      double calibOffset, DS18B20_RESOLUTION res, const OneWireBus* bus)
            : romCode(romCode), calibLinear(calibLinear), calibOffset(calibOffset),
              res(res), bus(bus) {}

        OneWireBus_ROMCode romCode {};
        double calibLinear = 0.0;
        double calibOffset = 0.0;
        DS18B20_RESOLUTION res = DS18B20_RESOLUTION::DS18B20_RESOLUTION_INVALID;
        const OneWireBus* bus = nullptr;
};

class Ds18b20
{
    static constexpr const char* Name = "Ds18b20";

    public:
        Ds18b20(void) = default;
        explicit Ds18b20(const Ds18b20Config& config);
        // Ds18b20(const PBDS18B20Sensor& serialConfig, DS18B20_RESOLUTION res, const OneWireBus* bus);
        
        // Update
        PBRet readTemp(float& temp) const;

        // Utility
        PBRet serialize(cJSON* root) const;
        static PBRet checkInputs(const Ds18b20Config& config);
        PBDS18B20Sensor toSerialConfig(void) const;

         // Operators
        bool operator ==(const Ds18b20& other) const;

        const DS18B20_Info& getInfo(void) const { return _info; }
        bool isConfigured(void) const { return _configured; }

    private:

        PBRet _initFromConfig(const Ds18b20Config& config);

        DS18B20_Info _info {};
        Ds18b20Config _config {};
        bool _configured = false;
};

#endif // PB_DS18B20_H