#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include "PBCommon.h"
#include "messageServer.h"
#include "CppTask.h"
#include "OneWireBus.h"

struct SensorManagerConfig
{
    double dt = 0.0;
    gpio_num_t oneWirePin = GPIO_NUM_NC;
    gpio_num_t refluxFlowPin = GPIO_NUM_NC;
    gpio_num_t productFlowPin = GPIO_NUM_NC;
    DS18B20_RESOLUTION tempSensorResolution = DS18B20_RESOLUTION_INVALID;
};

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

class SensorManager : public Task
{
    static constexpr const char* Name = "SensorManager";

    public:
        // Constructors
        SensorManager(UBaseType_t priority, UBaseType_t stackDepth, BaseType_t coreID, const SensorManagerConfig& cfg);

        static PBRet checkInputs(const SensorManagerConfig& cfg);
        bool isConfigured(void) const { return _configured; }

    private:

        // Initialization
        PBRet _initOneWireBus(const SensorManagerConfig& cfg) const;
        PBRet _initFlowmeters(const SensorManagerConfig& cfg) const;

        // Updates
        PBRet _readTemps(const TemperatureData& T);
        PBRet _readFlowmeters(const FlowrateData& F);

        // Setup methods
        PBRet _initFromParams(const SensorManagerConfig& cfg);
        PBRet _setupCBTable(void) override;

        // FreeRTOS hook method
        void taskMain(void) override;

        // Queue callbacks
        PBRet _generalMessageCB(std::shared_ptr<MessageBase> msg);

        // SensorManager data
        SensorManagerConfig _cfg {};

        // TODO: Sensor ID table
        PBOneWire _OWBus {};
        bool _configured = false;

};

#endif // SENSOR_MANAGER_H