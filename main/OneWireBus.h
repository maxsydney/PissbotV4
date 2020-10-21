#ifndef ONEWIRE_BUS_H
#define ONEWIRE_BUS_H

#include <vector>

#include "PBCommon.h"
#include "messageServer.h"
#include "PBds18b20.h"
#include "owb.h"
#include "owb_rmt.h"
#include "freertos/semphr.h"
#include "ds18b20.h"

class TemperatureData : public MessageBase
{
    static constexpr MessageType messageType = MessageType::TemperatureData;
    static constexpr const char* Name = "Temperature Data";

    public:
        TemperatureData(void) = default;
        TemperatureData(double headTemp, double refluxCondensorTemp, double prodCondensorTemp, 
                        double radiatorTemp, double boilerTemp)
            : MessageBase(TemperatureData::messageType, TemperatureData::Name), _headTemp(headTemp), 
            _refluxCondensorTemp(refluxCondensorTemp), _prodCondensorTemp(prodCondensorTemp), 
            _radiatorTemp(radiatorTemp), _boilerTemp(boilerTemp), _timeStamp(esp_timer_get_time()) {}

        double getHeadTemp(void) const { return _headTemp; }
        double getRefluxCondensorTemp(void) const { return _refluxCondensorTemp; }
        double getProdCondensorTemp(void) const { return _prodCondensorTemp; }
        double getRadiatorTemp(void) const { return _radiatorTemp; }
        double getBoilerTemp(void) const { return _boilerTemp; }
        int64_t getTimeStamp(void) const { return _timeStamp; }

    private:
        double _headTemp = 0.0;
        double _refluxCondensorTemp = 0.0;
        double _prodCondensorTemp = 0.0;
        double _radiatorTemp = 0.0;
        double _boilerTemp = 0.0;
        int64_t _timeStamp = 0;
};

// C++ wrapper around the esp32-owb library
// https://github.com/DavidAntliff/esp32-owb

class PBOneWire
{
    static constexpr const char* Name = "PBOneWire";
    static constexpr const char* FSBasePath = "/spiffs";
    static constexpr const char* FSPartitionLabel = "PBData";
    static constexpr const char* deviceFile = "/spiffs/devices.json";

    public:
        // Constructors
        PBOneWire(void) = default;
        explicit PBOneWire(gpio_num_t OWPin);

        // Initialisation
        PBRet scanForDevices(void);
        PBRet initialiseTempSensors(void);
        PBRet connect(void);

        // Update
        PBRet readTempSensors(TemperatureData& Tdata);

        static PBRet checkInputs(gpio_num_t OWPin);
        bool isConfigured(void) const { return _configured; }

    private:

        // Initialisation
        PBRet _initOWB(gpio_num_t OWPin);
        PBRet _loadKnownDevices(const char* basePath, const char* partitionLabel);

        // Update
        PBRet _readTempSensors(const TemperatureData& Tdata);
        PBRet _oneWireConvert(void) const;

        SemaphoreHandle_t _OWBMutex = NULL;
        OneWireBus* _owb = nullptr;
        owb_rmt_driver_info* _rmtDriver = nullptr;

        // Assigned sensors
        Ds18b20 _headTempSensor {};
        Ds18b20 _refluxTempSensor {};
        Ds18b20 _productTempSensor {};
        Ds18b20 _radiatorTempSensor {};
        Ds18b20 _boilerTempSensor {};

        // Unassigned sensors
        size_t _connectedDevices = 0;
        std::vector<OneWireBus_ROMCode> _availableRomCodes;     // TODO: Array of Ds18b20 objects
        std::vector<OneWireBus_ROMCode> _savedRomCodes;

        bool _configured = false;
};

#endif // ONEWIRE_BUS_H