#ifndef PB_DS18B20_H
#define PB_DS18B20_H

#include <vector>
#include "PBCommon.h"
#include "ds18b20.h"
#include "cJSON.h"

//
// Linear calibration for a Ds18b20 sensor
//
class Ds18b20Calibration
{
    public:
        Ds18b20Calibration(void) = default;
        Ds18b20Calibration(double A, double B)
            : A(A), B(B) {}

        double A = 1.0;     // Linear scaling coefficient
        double B = 0.0;     // Offset coefficient
};

class Ds18b20
{
    static constexpr const char* Name = "Ds18b20";
    static constexpr const char* calibrationStr = "calibration";
    static constexpr const char* romCodeStr = "romCode";
    static constexpr const char* calCoeffAStr = "calCoeffA";
    static constexpr const char* calCoeffBStr = "calCoeffB";

    public:
        Ds18b20(void) = default;
        Ds18b20(OneWireBus_ROMCode romCode, DS18B20_RESOLUTION res, 
                const OneWireBus *bus, const Ds18b20Calibration& cal);
        Ds18b20(const cJSON *JSONConfig, DS18B20_RESOLUTION res, 
                const OneWireBus *bus);

        // Update
        PBRet readTemp(double &temp) const;

        // Utility
        PBRet serialize(cJSON *root) const;
        const OneWireBus_ROMCode* getROMCode(void) const;
        const DS18B20_Info &getInfo(void) const { return _info; }
        static PBRet checkInputs(OneWireBus_ROMCode romCode, DS18B20_RESOLUTION res, 
                                const OneWireBus *bus, const Ds18b20Calibration& cal);
        bool isConfigured(void) const { return _configured; }

        // Operators
        bool operator==(const Ds18b20 &other) const;

    private:
        PBRet _initFromParams(OneWireBus_ROMCode romCode, DS18B20_RESOLUTION res, 
                            const OneWireBus *bus, const Ds18b20Calibration& cal);
        PBRet _initFromJSON(const cJSON* root, DS18B20_RESOLUTION res, const OneWireBus* bus);

        DS18B20_Info _info{};
        Ds18b20Calibration _cal {};
        bool _configured = false;
};

#endif // PB_DS18B20_H