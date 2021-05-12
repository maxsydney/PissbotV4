#ifndef ONEWIRE_BUS_H
#define ONEWIRE_BUS_H

#include <vector>

#include "OneWireBusMessaging.h"
#include "PBCommon.h"
#include "MessageServer.h"
#include "PBds18b20.h"
#include "owb.h"
#include "owb_rmt.h"
#include "freertos/semphr.h"
#include "ds18b20.h"

// C++ wrapper around the esp32-owb library
// https://github.com/DavidAntliff/esp32-owb

struct PBOneWireConfig
{
    gpio_num_t oneWirePin = (gpio_num_t)GPIO_NUM_NC;
    DS18B20_RESOLUTION tempSensorResolution = DS18B20_RESOLUTION_INVALID;
};

enum class SensorType
{
    Unknown,
    Head,
    Reflux,
    Product,
    Radiator,
    Boiler
};

class PBOneWire
{
    static constexpr const char *Name = "PBOneWire";
    static constexpr double MaxValidTemp = 110.0;
    static constexpr double MinValidTemp = -10.0;

public:
    // Constructors
    PBOneWire(void) = default;
    explicit PBOneWire(const PBOneWireConfig &cfg);

    // Update
    PBRet readTempSensors(TemperatureData &Tdata) const;

    // Get/Set
    PBRet setTempSensor(SensorType type, const Ds18b20 &sensor);
    PBRet getTempSensor(const OneWireBus_ROMCode& addr, Ds18b20& sensorOut);
    const OneWireBus *getOWB(void) const { return _owb; } // Probably not a great idea. Consider removing

    // Utility
    bool isAvailableSensor(const Ds18b20 &sensor) const;
    PBRet serialize(std::string &JSONstr) const;
    PBRet broadcastAvailableDevices(void);
    static SensorType mapSensorIDToType(int sensorID);

    static PBRet checkInputs(const PBOneWireConfig &cfg);
    static PBRet loadFromJSON(PBOneWireConfig &cfg, const cJSON *cfgRoot);
    bool isConfigured(void) const { return _configured; }

private:
    // Initialisation
    PBRet _initFromParams(const PBOneWireConfig &cfg);
    PBRet _initOWB();
    PBRet _loadKnownDevices(const char *basePath, const char *partitionLabel);

    // Update
    PBRet _readTempSensors(const TemperatureData &Tdata);
    PBRet _oneWireConvert(void) const;

    // Utility
    PBRet _writeToFile(void) const;
    PBRet _printConfigFile(void) const;
    PBRet _scanForDevices(void);
    PBRet _broadcastDeviceAddresses(void) const;

    SemaphoreHandle_t _OWBMutex = NULL;
    OneWireBus *_owb = nullptr;
    owb_rmt_driver_info *_rmtDriver = nullptr;

    // TODO: This interface could be cleaned up a lot by using key-val mapping between
    // assigned task and sensor object
    // Assigned sensors
    Ds18b20 _headTempSensor{};
    Ds18b20 _refluxTempSensor{};
    Ds18b20 _productTempSensor{};
    Ds18b20 _radiatorTempSensor{};
    Ds18b20 _boilerTempSensor{};

    // Unassigned sensors
    std::vector<Ds18b20> _availableSensors;

    // Class data
    PBOneWireConfig _cfg{};
    bool _configured = false;
};

#endif // ONEWIRE_BUS_H