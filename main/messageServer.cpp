#include "messageServer.h"
#include "MessageDefs.h"
#include <esp_log.h>

std::vector<Subscriber> MessageServer::_subscribers {};

PBRet MessageServer::registerTask(const Subscriber& subscriber)
{
    // TODO: How to check if queue is initialized? Write C++ wrapper?
    _subscribers.push_back(subscriber);

    ESP_LOGI(MessageServer::Name, "Registered new subscriber: %s", subscriber.getName());

    return PBRet::SUCCESS;
}

PBRet MessageServer::broadcastMessage(const MessageBase* message)
{
    // Broadcast message to the general purpose queue of any subscribing tasks
    if (message == nullptr) {
        ESP_LOGW(MessageServer::Name, "Message pointer was null");
    }

    MessageType msgType = message->getType();

    if (msgType == MessageType::Unknown) {
        ESP_LOGE(MessageServer::Name, "Message type was unknown");
        return PBRet::FAILURE;
    }

    for (const Subscriber& subscriber: _subscribers) {
        if (subscriber.isSubscribed(msgType)) {
            // TODO: Ensure this is deleted.
            BaseType_t ret = xQueueSend(subscriber.getQueueHandle(), &message, 100 / portTICK_PERIOD_MS);
            if (ret != pdTRUE) {
                ESP_LOGW(MessageServer::Name, "Unable to add message to %s queue", subscriber.getName());
            }
        }
    }

    return PBRet::SUCCESS;
}

bool Subscriber::isSubscribed(MessageType msgType) const
{
    return (_subscriptions.find(msgType) != _subscriptions.end());
}