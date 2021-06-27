#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include "SensorManagerMessaging.h"
#include "PBCommon.h"
#include "CppTask.h"
#include "OneWireBus.h"
#include "Flowmeter.h"

// Forward declarations
class PBOneWire;

struct SensorManagerConfig
{
    double dt = 0.0;
    PBOneWireConfig oneWireConfig{};
    FlowmeterConfig refluxFlowConfig{};
    FlowmeterConfig productFlowConfig{};
};

class SensorManager : public Task
{
    static constexpr const char *Name = "SensorManager";
    static constexpr const char *FSBasePath = "/spiffs";
    static constexpr const char *FSPartitionLabel = "PBData";
    static constexpr const char *deviceFile = "/spiffs/devices.json";

public:
    // Constructors
    SensorManager(UBaseType_t priority, UBaseType_t stackDepth, BaseType_t coreID, const SensorManagerConfig &cfg);

    static PBRet checkInputs(const SensorManagerConfig &cfg);
    static PBRet loadFromJSON(SensorManagerConfig &cfg, const cJSON *cfgRoot);
    bool isConfigured(void) const { return _configured; }

    friend class SensorManagerUT;

    static constexpr const char *HeadTempSensorKey = "headTempSensor";
    static constexpr const char *RefluxTempSensorKey = "refluxTempSensor";
    static constexpr const char *ProductTempSensorKey = "productTempSensor";
    static constexpr const char *RadiatorTempSensorKey = "radiatorTempSensor";
    static constexpr const char *BoilerTempSensorKey = "boilerTempSensor";

private:
    // Initialization
    PBRet _initOneWireBus(const SensorManagerConfig &cfg);
    PBRet _initFromParams(const SensorManagerConfig &cfg);
    PBRet _setupCBTable(void) override;
    PBRet _loadKnownDevices(const char *basePath, const char *partitionLabel);
    PBRet _loadTempSensorsFromJSON(const cJSON *JSONTempSensors);

    // Updates
    PBRet _readFlowmeters(const FlowrateData &F) const;
    PBRet _estimateABV(const TemperatureData &TData, ConcentrationData& concData) const;
    PBRet _broadcastTemps(const TemperatureData &Tdata) const;
    PBRet _broadcastFlowrates(const FlowrateData &flowrateData) const;
    PBRet _broadcastConcentrations(const ConcentrationData& concData) const;

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
    PBRet _calibrateSensorCB(std::shared_ptr<MessageBase> msg);

    // SensorManager data
    SensorManagerConfig _cfg{};

    // Connected sensors
    PBOneWire _OWBus{};
    Flowmeter _refluxFlowmeter{};
    Flowmeter _productFlowmeter{};

    // Class data
    bool _configured = false;
};

#endif // SENSOR_MANAGER_H