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
#include "cJSON.h"
#include "PBCommon.h"
#include "Generated/MessageBase.h"

constexpr uint32_t MESSAGE_SIZE = 256;
using PBMessageWrapper = MessageWrapper<MESSAGE_SIZE>;

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
    ControllerState,
    SocketLog
};

// Represents a task and the messages that it is subscribed to
class Subscriber
{
    public:
        Subscriber(const char* name, std::queue<std::shared_ptr<PBMessageWrapper>>& taskQueue, const std::set<PBMessageType>& subscriptions)
            : _name(name), _taskQueue(taskQueue), _subscriptions(subscriptions) {}

        bool isSubscribed(PBMessageType msgType) const;                       // Returns true if subscriber is subscribed to msgType
        const char* getName(void) const { return _name; }
        std::queue<std::shared_ptr<PBMessageWrapper>>& getQueueHandle(void) const { return _taskQueue; }
        
    private:
        const char* _name = nullptr;
        std::queue<std::shared_ptr<PBMessageWrapper>>& _taskQueue;       // TODO: Is this legit?
        std::set<PBMessageType> _subscriptions;
        PBMessageWrapper _wrapper {};
};

class MessageServer
{
    static constexpr const char* Name = "Message Server";

    public:
        static PBRet registerTask(const Subscriber& subscriber);
        static PBRet broadcastMessage(const PBMessageWrapper& message);
        static PBMessageWrapper wrap(const ::EmbeddedProto::MessageInterface& message, PBMessageType type, MessageOrigin origin);
        static PBRet unwrap(const PBMessageWrapper& wrapped, ::EmbeddedProto::MessageInterface& message);
        static void printErr(::EmbeddedProto::Error err);

    private:
        static std::vector<Subscriber> _subscribers;
};

#endif // MESSAGE_SERVER_H