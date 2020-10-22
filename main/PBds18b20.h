#ifndef PB_DS18B20_H
#define PB_DS18B20_H

#include "PBCommon.h"
#include "ds18b20.h"
#include "cJSON.h"

class Ds18b20
{
    static constexpr const char* Name = "Ds18b20";

    public:
        Ds18b20(void) = default;
        Ds18b20(OneWireBus_ROMCode romCode, DS18B20_RESOLUTION res, const OneWireBus* bus);
        Ds18b20(const cJSON* JSONConfig, DS18B20_RESOLUTION res, const OneWireBus* bus);

        // Update
        PBRet readTemp(float& temp);

        // Utility
        PBRet serialize(cJSON* root) const;
        static PBRet checkInputs(OneWireBus_ROMCode romCode, DS18B20_RESOLUTION res, const OneWireBus* bus);

         // Operators
        bool operator ==(const Ds18b20& other) const;

        const DS18B20_Info& getInfo(void) const { return _info; }
        bool isConfigured(void) const { return _configured; }

    private:

        PBRet _initFromParams(OneWireBus_ROMCode romCode, DS18B20_RESOLUTION res, const OneWireBus* bus);

        DS18B20_Info _info {};
        bool _configured = false;
};

#endif // PB_DS18B20_H