#ifndef MAIN_ONEWIREBUS_MESSAGING_H
#define MAIN_ONEWIREBUS_MESSAGING_H

#include "PBCommon.h"
#include "PBds18b20.h"
#include "MessageDefs.h"
#include "cJSON.h"

class TemperatureData : public MessageBase
{
    static constexpr PBMessageType messageType = PBMessageType::TemperatureData;
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
    static constexpr PBMessageType messageType = PBMessageType::DeviceData;
    static constexpr const char *Name = "OneWireBus Device Data";

public:
    DeviceData(void) = default;
    DeviceData(const std::vector<Ds18b20> &devices)
        : MessageBase(DeviceData::messageType, DeviceData::Name, esp_timer_get_time()), _devices(devices) {}

    const std::vector<Ds18b20> &getDevices(void) const { return _devices; }

    PBRet serialize(std::string &JSONStr) const override { return PBRet::FAILURE; }
    PBRet deserialize(const cJSON *root) override { return PBRet::FAILURE; }

private:
    std::vector<Ds18b20> _devices{};
};

#endif // MAIN_ONEWIREBUS_MESSAGING_H