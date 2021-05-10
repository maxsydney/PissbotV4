#ifndef PB_DS18B20_H
#define PB_DS18B20_H

#include <vector>
#include "PBCommon.h"
#include "ds18b20.h"
#include "cJSON.h"

//
// Calibration for a Ds18b20 sensor. Calibration is stored
// as a set of polynomial coefficients to allow for nonlinear
// calibration if required
//
class Ds18b20Calibration
{
    public:
        Ds18b20Calibration(void) = default;
        Ds18b20Calibration(const std::vector<double>& calCoeff)
            : calCoeff(calCoeff) {}

        std::vector<double> calCoeff = {1.0, 0.0};       // Calibration coefficients

        static constexpr uint8_t MAX_CAL_LEN = 4;   // Up to cubic only
};

class Ds18b20
{
    static constexpr const char *Name = "Ds18b20";

    public:
        Ds18b20(void) = default;
        Ds18b20(OneWireBus_ROMCode romCode, DS18B20_RESOLUTION res, 
                const OneWireBus *bus, const Ds18b20Calibration& cal);
        Ds18b20(const cJSON *JSONConfig, DS18B20_RESOLUTION res, 
                const OneWireBus *bus);

        // Update
        PBRet readTemp(float &temp) const;

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