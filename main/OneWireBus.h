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
    static constexpr const char *Name = "Temperature Data";
    static constexpr const char *HeadTempStr = "HeadTemp";
    static constexpr const char *RefluxCondensorTempStr = "RefluxTemp";
    static constexpr const char *ProdCondensorTempStr = "ProdTemp";
    static constexpr const char *RadiatorTempStr = "RadiatorTemp";
    static constexpr const char *BoilerTempStr = "BoilerTemp";
    static constexpr const char *UptimeStr = "Uptime";

public:
    TemperatureData(void) = default;
    TemperatureData(double headTemp, double refluxCondensorTemp, double prodCondensorTemp,
                    double radiatorTemp, double boilerTemp)
        : MessageBase(TemperatureData::messageType, TemperatureData::Name, esp_timer_get_time()), headTemp(headTemp),
          refluxCondensorTemp(refluxCondensorTemp), prodCondensorTemp(prodCondensorTemp),
          radiatorTemp(radiatorTemp), boilerTemp(boilerTemp) {}

    PBRet serialize(std::string &JSONStr) const;
    PBRet deserialize(const cJSON *root) { return PBRet::SUCCESS; }

    double headTemp = 0.0;
    double refluxCondensorTemp = 0.0;
    double prodCondensorTemp = 0.0;
    double radiatorTemp = 0.0;
    double boilerTemp = 0.0;
};

class DeviceData : public MessageBase
{
    static constexpr MessageType messageType = MessageType::DeviceData;
    static constexpr const char *Name = "OneWireBus Device Data";

public:
    DeviceData(void) = default;
    DeviceData(const std::vector<Ds18b20> &devices)
        : MessageBase(DeviceData::messageType, DeviceData::Name, esp_timer_get_time()), _devices(devices) {}

    const std::vector<Ds18b20> &getDevices(void) const { return _devices; }

private:
    std::vector<Ds18b20> _devices{};
};

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

public:
    // Constructors
    PBOneWire(void) = default;
    explicit PBOneWire(const PBOneWireConfig &cfg);

    // Initialisation
    PBRet initialiseTempSensors(void);

    // Update
    PBRet readTempSensors(TemperatureData &Tdata) const;

    // Get/Set
    PBRet setTempSensor(SensorType type, const Ds18b20 &sensor);
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
    size_t _connectedDevices = 0;
    std::vector<Ds18b20> _availableSensors;

    // Class data
    PBOneWireConfig _cfg{};
    bool _configured = false;
};

#endif // ONEWIRE_BUS_H