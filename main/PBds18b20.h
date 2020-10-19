#ifndef PB_DS18B20_H
#define PB_DS18B20_H

#include "PBCommon.h"
#include "ds18b20.h" 

class Ds18b20
{
    static constexpr const char* Name = "Ds18b20";

    public:
        Ds18b20(void) = default;
        Ds18b20(OneWireBus_ROMCode romCode, DS18B20_RESOLUTION res, const OneWireBus* bus);

        static PBRet checkInputs(OneWireBus_ROMCode romCode, DS18B20_RESOLUTION res, const OneWireBus* bus);

    private:

        DS18B20_Info _info {};
        bool _configured = false;
};

#endif // PB_DS18B20_H