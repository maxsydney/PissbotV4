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
#include "PBCommon.h"

enum class MessageType {
    Unknown,
    General,
    TemperatureData,
    FlowrateData,
    TuningParameters,
    ControlCommand
};

// Represents a task and the messages that it is subscribed to
class Subscriber
{
    public:
        Subscriber(const char* name, xQueueHandle taskQueue, const std::set<MessageType>& subscriptions)
            : _name(name), _taskQueue(taskQueue), _subscriptions(subscriptions) {}

        bool isSubscribed(MessageType msgType) const;                       // Returns true if subscriber is subscribed to msgType
        const char* getName(void) const { return _name; }
        xQueueHandle getQueueHandle(void) const { return _taskQueue; }
        
    private:
        const char* _name = nullptr;
        xQueueHandle _taskQueue;
        std::set<MessageType> _subscriptions;
};

// Base class for all messages to be passed over network
class MessageBase
{
    // TODO: How to make sure this data will be deleted?
    public:
        MessageBase(MessageType msgType);

        MessageType getType(void) const { return _type; }

        // Add payload in derived classes
    
    private:
        MessageType _type = MessageType::Unknown;
};

class MessageServer
{
    static constexpr char* tag = "Message Server";

    public:
        static PBRet registerTask(const Subscriber& subscriber);
        static PBRet broadcastMessage(const MessageBase& message);

    private:
        static std::vector<Subscriber> _subscribers;
};

#endif // MESSAGE_SERVER_H
