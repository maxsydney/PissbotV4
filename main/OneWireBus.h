#ifndef ONEWIRE_BUS_H
#define ONEWIRE_BUS_H

#include <vector>
#include <memory>
#include <unordered_map>
#include "PBCommon.h"
#include "MessageServer.h"
#include "PBds18b20.h"
#include "owb.h"
#include "owb_rmt.h"
#include "freertos/semphr.h"
#include "ds18b20.h"
#include "Generated/SensorManagerMessaging.h"
#include "Generated/DS18B20Messaging.h"

// C++ wrapper around the esp32-owb library
// https://github.com/DavidAntliff/esp32-owb

constexpr uint8_t DEVICE_DATA_LEN = 12;
using PBDeviceData = DeviceData<DEVICE_DATA_LEN, ROM_SIZE>;
using SensorMap = std::unordered_map<DS18B20Role, std::shared_ptr<Ds18b20>> ;

struct PBOneWireConfig
{
    gpio_num_t oneWirePin = (gpio_num_t)GPIO_NUM_NC;
    DS18B20_RESOLUTION tempSensorResolution = DS18B20_RESOLUTION_INVALID;
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

    // Initialisation
    PBRet initialiseTempSensors(void);

    // Update
    PBRet readTempSensors(TemperatureData &Tdata) const;

    // Get/Set
    PBRet setTempSensor(DS18B20Role type, const std::shared_ptr<Ds18b20>& sensor);
    const OneWireBus *getOWB(void) const { return _owb; } // Probably not a great idea. Consider removing

    // Utility
    bool isAvailableSensor(const Ds18b20 &sensor) const;
    PBRet serialize(std::string &JSONstr) const;
    PBRet broadcastAvailableDevices(void);

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
    PBRet _readTemperatureSensor(DS18B20Role sensor, double& T) const;

    SemaphoreHandle_t _OWBMutex = NULL;
    OneWireBus *_owb = nullptr;
    owb_rmt_driver_info *_rmtDriver = nullptr;

    // Assigned sensors
    SensorMap _assignedSensors {};

    // Store all identified sensors on the network
    std::vector<std::shared_ptr<Ds18b20>> _availableSensors {};

    // Class data
    PBOneWireConfig _cfg{};
    bool _configured = false;
};

#endif // ONEWIRE_BUS_H