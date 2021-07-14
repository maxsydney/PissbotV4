#ifndef MAIN_SENSORMANAGER_MESSAGING_H
#define MAIN_SENSORMANAGER_MESSAGING_H

#include "PBCommon.h"
#include "MessageDefs.h"
#include "cJSON.h"
#include "OneWireBus.h"
#include "owb.h"

enum class SensorManagerCmdType
{
    None,
    BroadcastSensorsStart,
    BroadcastSensorsStop
};

class SensorManagerCommand : public MessageBase
{
    static constexpr PBMessageType messageType = PBMessageType::SensorManagerCommand;
    static constexpr const char *Name = "Sensor Manager Command";

public:
    SensorManagerCommand(void) = default;
    SensorManagerCommand(SensorManagerCmdType cmdType)
        : MessageBase(SensorManagerCommand::messageType, SensorManagerCommand::Name, esp_timer_get_time()),
          _cmdType(cmdType) {}

    SensorManagerCmdType getCommandType(void) const { return _cmdType; }

    PBRet serialize(std::string &JSONStr) const override;
    PBRet deserialize(const cJSON *root) override;

private:
    SensorManagerCmdType _cmdType = SensorManagerCmdType::None;
};

class FlowrateData : public MessageBase
{
    static constexpr PBMessageType messageType = PBMessageType::FlowrateData;
    static constexpr const char *Name = "Flowrate Data";

    static constexpr const char *RefluxFlowrateStr = "refluxFlowrate";
    static constexpr const char *ProductFlowrateStr = "productFlowrate";

public:
    FlowrateData(void) = default;
    FlowrateData(double refluxFlowrate, double productFlowrate)
        : MessageBase(FlowrateData::messageType, FlowrateData::Name, esp_timer_get_time()), refluxFlowrate(refluxFlowrate),
          productFlowrate(productFlowrate) {}

    PBRet serialize(std::string &JSONStr) const override;
    PBRet deserialize(const cJSON *root) override;

    double refluxFlowrate = 0.0;  // [kg / s]
    double productFlowrate = 0.0; // [kg / s]
};

class ConcentrationData : public MessageBase
{
    static constexpr PBMessageType messageType = PBMessageType::ConcentrationData;
    static constexpr const char *Name = "Concentration Data";

    static constexpr const char *VapourConcStr = "vapourConc";
    static constexpr const char *BoilerConcStr = "boilerConc";

public:
    ConcentrationData(void) = default;
    ConcentrationData(double vapourConc, double boilerConc)
        : MessageBase(ConcentrationData::messageType, ConcentrationData::Name, esp_timer_get_time()),
          vapourConcentration(vapourConc), boilerConcentration(boilerConc) {}

    PBRet serialize(std::string &JSONStr) const override;
    PBRet deserialize(const cJSON *root) override;

    double vapourConcentration = 0.0;
    double boilerConcentration = 0.0;
};

class AssignSensorCommand : public MessageBase
{
    static constexpr PBMessageType messageType = PBMessageType::AssignSensor;
    static constexpr const char *Name = "Assign Sensor";

public:
    AssignSensorCommand(void) = default;
    AssignSensorCommand(const OneWireBus_ROMCode &addr, SensorType sensorType)
        : MessageBase(AssignSensorCommand::messageType, AssignSensorCommand::Name, esp_timer_get_time()),
          _address(addr), _sensorType(sensorType) {}

    const OneWireBus_ROMCode &getAddress(void) const { return _address; }
    SensorType getSensorType(void) const { return _sensorType; }

    PBRet serialize(std::string &JSONStr) const override;
    PBRet deserialize(const cJSON *root) override;

private:
    OneWireBus_ROMCode _address{};
    SensorType _sensorType = SensorType::Unknown;
};

#endif // MAIN_SENSORMANAGER_MESSAGING_H