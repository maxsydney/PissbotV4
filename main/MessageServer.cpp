#include "MessageServer.h"
#include "MessageDefs.h"
#include "IO/Writable.h"
#include "IO/Readable.h"
#include <esp_log.h>

std::vector<Subscriber> MessageServer::_subscribers {};

PBRet MessageServer::registerTask(const Subscriber& subscriber)
{
    // TODO: How to check if queue is initialized? Write C++ wrapper?
    _subscribers.push_back(subscriber);

    ESP_LOGI(MessageServer::Name, "Registered new subscriber: %s", subscriber.getName());

    return PBRet::SUCCESS;
}

PBRet MessageServer::broadcastMessage(const PBMessageWrapper& message)
{
    // Broadcast message to the general purpose queue of any subscribing tasks

    PBMessageType msgType = message.get_type();

    if (msgType == PBMessageType::Unknown) {
        ESP_LOGE(MessageServer::Name, "Message type was unknown");
        return PBRet::FAILURE;
    }

    for (const Subscriber& subscriber: _subscribers) {
        if (subscriber.isSubscribed(msgType)) {
            // TODO: Ensure this is deleted.
            subscriber.getQueueHandle().push(std::make_shared<PBMessageWrapper>(message));
        }
    }

    return PBRet::SUCCESS;
}

bool Subscriber::isSubscribed(PBMessageType msgType) const
{
    return (_subscriptions.find(msgType) != _subscriptions.end());
}

PBMessageWrapper MessageServer::wrap(const ::EmbeddedProto::MessageInterface& message, PBMessageType type)
{
    // Wrap a protobuf message in a PBMessageWrapper to transmit out over the network

    // Create an empty wrapper and set the type
    // TODO: Auto type association?
    PBMessageWrapper wrapper {};
    wrapper.set_type(type);

    // Serialize the message to a buffer
    Writable buffer {};
    message.serialize(buffer);

    // Write the serialized message into the payload of the wrapper
    wrapper.mutable_payload().set(buffer.get_buffer(), buffer.get_size());

    return wrapper;
}

PBRet MessageServer::unwrap(const PBMessageWrapper& wrapped, ::EmbeddedProto::MessageInterface& message)
{
    // Unwrap a protobuf message into a specific message type
    // TODO: Error checking

    Readable readBuffer {};
    for (size_t i = 0; i < wrapped.get_payload().get_length(); i++)
    {
        uint8_t ch = static_cast<uint8_t>(wrapped.get_payload().get_const(i));
        readBuffer.push(ch);
    }

    message.deserialize(readBuffer);

    return PBRet::SUCCESS;
}