// Publish/subsribe model for tasks

// - Each task gets a general purpose queue when started. Data passed must be general
// - Any task can send data to server, which then broadcasts the message into the general purpose queue of all intended recipients
// - Server maintains a list of registered tasks, and the types of messages that it is subscribed to
// - Receiving task defines handler for each message it is subsribed to - msgtype - std::function map
// - Queue type is pointer to base message type
// - Base type is purely virtual
// - Specific messages inherit from messageBase

// TODO: Add tests

#ifndef MESSAGE_SERVER_H
#define MESSAGE_SERVER_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include <vector>
#include <set>
#include <memory>
#include <queue>
#include "PBCommon.h"

enum class MessageType {
    Unknown,
    General,
    TemperatureData,
    FlowrateData,
    ControlTuning,
    ControlCommand,
    ControlSettings,
    ControllerDataRequest,
    SensorManagerCmd,
    DeviceData,
    AssignSensor,
    ConcentrationData,
    ControllerState
};

// Base class for all messages to be passed over network
class MessageBase
{   
    public:
        MessageBase(void) = default;
        MessageBase(MessageType msgType, const std::string& name, int64_t timeStamp)
            : _type(msgType), _name(name), _timeStamp(timeStamp), _valid(true) {}

        MessageType getType(void) const { return _type; }
        bool isValid(void) const { return _valid; }
        const std::string& getName(void) const { return _name; }
        int64_t getTimeStamp(void) const { return _timeStamp; }

    protected:
        static constexpr const char* TimeStampStr = "timestamp";
        MessageType _type = MessageType::Unknown;
        std::string _name {};
        int64_t _timeStamp = 0;             // [uS]
        bool _valid = false;
};

// Represents a task and the messages that it is subscribed to
class Subscriber
{
    public:
        Subscriber(const char* name, std::queue<std::shared_ptr<MessageBase>>& taskQueue, const std::set<MessageType>& subscriptions)
            : _name(name), _taskQueue(taskQueue), _subscriptions(subscriptions) {}

        bool isSubscribed(MessageType msgType) const;                       // Returns true if subscriber is subscribed to msgType
        const char* getName(void) const { return _name; }
        std::queue<std::shared_ptr<MessageBase>>& getQueueHandle(void) const { return _taskQueue; }
        
    private:
        const char* _name = nullptr;
        std::queue<std::shared_ptr<MessageBase>>& _taskQueue;       // TODO: Is this legit?
        std::set<MessageType> _subscriptions;
};

class MessageServer
{
    static constexpr const char* Name = "Message Server";

    public:
        static PBRet registerTask(const Subscriber& subscriber);
        static PBRet broadcastMessage(const std::shared_ptr<MessageBase>& message);

    private:
        static std::vector<Subscriber> _subscribers;
};

#endif // MESSAGE_SERVER_H
