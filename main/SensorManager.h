#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include "PBCommon.h"
#include "CppTask.h"
#include "OneWireBus.h"

// Forward declarations
class PBOneWire;

struct SensorManagerConfig
{
    double dt = 0.0;
    PBOneWireConfig oneWireConfig {};
};

enum class SensorManagerCmdType { None, BroadcastSensorsStart, BroadcastSensorsStop};

class SensorManagerCommand : public MessageBase
{
    static constexpr MessageType messageType = MessageType::SensorManagerCmd;
    static constexpr const char* Name = "Sensor Manager Command";

    public:
        SensorManagerCommand(void) = default;
        SensorManagerCommand(SensorManagerCmdType cmdType)
            : MessageBase(SensorManagerCommand::messageType, SensorManagerCommand::Name),
              _cmdType(cmdType) {}

        SensorManagerCmdType getCommandType(void) const { return _cmdType; }
    
    private:

        SensorManagerCmdType _cmdType = SensorManagerCmdType::None;
};

class FlowrateData : public MessageBase
{
    static constexpr MessageType messageType = MessageType::FlowrateData;
    static constexpr const char* Name = "Flowrate Data";

    public:
        FlowrateData(void) = default;
        FlowrateData(double refluxFlowrate, double productFlowrate)
            : MessageBase(FlowrateData::messageType, FlowrateData::Name), _refluxFlowrate(refluxFlowrate), 
              _productFlowrate(productFlowrate), _timeStamp(esp_timer_get_time()) {}

        double getRefluxFlowrate(void) const { return _refluxFlowrate; }
        double getProductFlowrate(void) const { return _productFlowrate; }
        int64_t getTimeStamp(void) const { return _timeStamp; }

    private:
        double _refluxFlowrate = 0.0;       // [kg / s]
        double _productFlowrate = 0.0;      // [kg / s]
        int64_t _timeStamp = 0;             // [uS]
};

class AssignSensorCommand : public MessageBase
{
    static constexpr MessageType messageType = MessageType::AssignSensor;
    static constexpr const char* Name = "Assign Sensor";

    public:
        AssignSensorCommand(void) = default;
        AssignSensorCommand(const OneWireBus_ROMCode& addr, SensorType sensorType)
            : MessageBase(AssignSensorCommand::messageType, AssignSensorCommand::Name),
              _address(addr), _sensorType(sensorType) {}
        
        const OneWireBus_ROMCode& getAddress(void) const { return _address; }
        SensorType getSensorType(void) const { return _sensorType; }

    private:

        OneWireBus_ROMCode _address {};
        SensorType _sensorType = SensorType::Unknown;
};

class SensorManager : public Task
{
    static constexpr const char* Name = "SensorManager";
    static constexpr const char* FSBasePath = "/spiffs";
    static constexpr const char* FSPartitionLabel = "PBData";
    static constexpr const char* deviceFile = "/spiffs/devices.json";

    public:
        // Constructors
        SensorManager(UBaseType_t priority, UBaseType_t stackDepth, BaseType_t coreID, const SensorManagerConfig& cfg);

        static PBRet checkInputs(const SensorManagerConfig& cfg);
        bool isConfigured(void) const { return _configured; }

        friend class SensorManagerUT;

        static constexpr const char* HeadTempSensorKey = "headTempSensor";
        static constexpr const char* RefluxTempSensorKey = "refluxTempSensor";
        static constexpr const char* ProductTempSensorKey = "productTempSensor";
        static constexpr const char* RadiatorTempSensorKey = "radiatorTempSensor";
        static constexpr const char* BoilerTempSensorKey = "boilerTempSensor";

    private:

        // Initialization
        PBRet _initOneWireBus(const SensorManagerConfig& cfg) const;
        PBRet _initFlowmeters(const SensorManagerConfig& cfg) const;
        PBRet _initFromParams(const SensorManagerConfig& cfg);
        PBRet _setupCBTable(void) override;
        PBRet _loadKnownDevices(const char* basePath, const char* partitionLabel);
        PBRet _loadTempSensorsFromJSON(const cJSON* JSONTempSensors);
        PBRet _loadFlowmetersFromJSON(const cJSON* JSONFlowmeters);

        // Updates
        PBRet _readFlowmeters(const FlowrateData& F) const;
        PBRet _broadcastTemps(const TemperatureData& Tdata) const;

        // Utilities
        PBRet _writeSensorConfigToFile(void) const;
        PBRet _printConfigFile(void) const;
        PBRet _broadcastSensors(void);

        // FreeRTOS hook method
        void taskMain(void) override;

        // Queue callbacks
        PBRet _generalMessageCB(std::shared_ptr<MessageBase> msg);
        PBRet _commandMessageCB(std::shared_ptr<MessageBase> msg);
        PBRet _assignSensorCB(std::shared_ptr<MessageBase> msg);

        // SensorManager data
        SensorManagerConfig _cfg {};

        // TODO: Sensor ID table
        PBOneWire _OWBus {};

        // Class data
        bool _configured = false;
};

#endif // SENSOR_MANAGER_H