#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include "PBCommon.h"
#include "messageServer.h"
#include "CppTask.h"

class TemperatureData : public MessageBase
{
    static constexpr MessageType messageType = MessageType::TemperatureData;
    static constexpr const char* Name = "Temperature Data";

    public:
        TemperatureData(void) = default;
        TemperatureData(double headTemp, double refluxCondensorTemp, double prodCondensorTemp, 
                        double radiatorTemp, double boilerTemp, int64_t timeStamp)
            : MessageBase(TemperatureData::messageType, TemperatureData::Name), _headTemp(headTemp), 
            _refluxCondensorTemp(refluxCondensorTemp), _prodCondensorTemp(prodCondensorTemp), 
            _radiatorTemp(radiatorTemp), _boilerTemp(boilerTemp), _timeStamp(timeStamp) {}

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

class SensorManager : public Task
{
    // TODO: Implement this
};

#endif // SENSOR_MANAGER_H