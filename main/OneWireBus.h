#ifndef ONEWIRE_BUS_H
#define ONEWIRE_BUS_H

#include <vector>

#include "PBCommon.h"
#include "owb.h"
#include "owb_rmt.h"
#include "ds18b20.h"

// C++ wrapper around the esp32-owb library
// https://github.com/DavidAntliff/esp32-owb

class PBOneWire
{
    static constexpr const char* Name = "PBOneWire";

    public:
        // Constructors
        PBOneWire(void) = default;
        explicit PBOneWire(gpio_num_t OWPin);

        PBRet scanForDevices(void);

        static PBRet checkInputs(gpio_num_t OWPin);
        bool isConfigured(void) const { return _configured; }

    private:

        PBRet _initOWB(gpio_num_t OWPin);

        OneWireBus* _owb = nullptr;
        owb_rmt_driver_info _rmt_driver_info {};
        size_t _connectedDevices = 0;
        std::vector<OneWireBus_ROMCode> _availableRomCodes;
        std::vector<OneWireBus_ROMCode> _savedRomCodes;

        bool _configured = false;
};

#endif // ONEWIRE_BUS_H