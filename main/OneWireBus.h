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
#include "IO/Writable.h"
#include "IO/Readable.h"
#include "Generated/SensorManagerMessaging.h"
#include "Generated/DS18B20Messaging.h"

// C++ wrapper around the esp32-owb library
// https://github.com/DavidAntliff/esp32-owb

constexpr uint8_t DEVICE_DATA_LEN = 12;
using PBDeviceData = DeviceData<DEVICE_DATA_LEN, ROM_SIZE>;
using SensorMap = std::unordered_map<DS18B20Role, std::shared_ptr<Ds18b20>>;
using DeviceVector = std::vector<OneWireBus_ROMCode>;

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

    // Update
    PBRet readTempSensors(TemperatureData &Tdata) const;

    // Get/Set
    PBRet setTempSensor(DS18B20Role type, const std::shared_ptr<Ds18b20>& sensor);
    const OneWireBus *getOWB(void) const { return _owb; } // Probably not a great idea. Consider removing

    // Utility
    PBRet serialize(Writable& buffer) const;
    PBRet deserialize(Readable& buffer);
    PBRet broadcastAvailableDevices(void) const;

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
    PBRet _scanForDevices(DeviceVector& devices) const;
    PBRet _broadcastDeviceAddresses(const DeviceVector& deviceAddresses) const;
    PBRet _readTemperatureSensor(DS18B20Role sensor, double& T) const;
    static bool _isAvailableSensor(const PBDS18B20Sensor& sensor, const DeviceVector& deviceAddresses);
    static bool _romCodesMatch(const OneWireBus_ROMCode& a, const OneWireBus_ROMCode& b);
    PBRet _createAndAssignSensor(const PBDS18B20Sensor& sensorConfig, DS18B20Role role, DS18B20_RESOLUTION res, const std::string& name);

    SemaphoreHandle_t _OWBMutex = NULL;
    OneWireBus *_owb = nullptr;
    owb_rmt_driver_info *_rmtDriver = nullptr;

    // Assigned sensors
    SensorMap _assignedSensors {};

    // Class data
    PBOneWireConfig _cfg{};
    bool _configured = false;
};

#endif // ONEWIRE_BUS_H