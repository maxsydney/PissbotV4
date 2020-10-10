#ifndef MAIN_MESSAGE_DEFS_H
#define MAIN_MESSAGE_DEFS_H

#include <string>
#include "messageServer.h"

class GeneralMessage : public MessageBase
{
    static constexpr MessageType messageType = MessageType::General;
    static constexpr const char* Name = "General Message";

    public:
        GeneralMessage(void) = default;
        GeneralMessage(const std::string& message)
            : MessageBase(GeneralMessage::messageType, GeneralMessage::Name), _message(message) {}

        const std::string& getMessage(void) const { return _message; }
        
    private:
        std::string _message {};

};

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

#endif // MAIN_MESSAGE_DEFS_H