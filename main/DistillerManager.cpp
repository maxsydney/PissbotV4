#include <stdio.h>
#include "DistillerManager.h"
#include "messageServer.h"
#include "MessageDefs.h"

DistillerManager* DistillerManager::_managerPtr = nullptr;

DistillerManager* DistillerManager::getInstance(UBaseType_t priority, UBaseType_t stackDepth, BaseType_t coreID, const DistillerConfig& cfg)
{
    if (_managerPtr == nullptr) {
        _managerPtr = new DistillerManager(priority, stackDepth, coreID, cfg);
    }

    return _managerPtr;
}

DistillerManager* DistillerManager::getInstance(void)
{
    if (_managerPtr == nullptr) {
        printf("DistillerManager not initialized. Call getDistillerManager with arguments\n");
        return nullptr;
    }

    return _managerPtr;
}

DistillerManager::DistillerManager(UBaseType_t priority, UBaseType_t stackDepth, BaseType_t coreID, const DistillerConfig& cfg)
    : Task(DistillerManager::Name, priority, stackDepth, coreID, NULL)
{
    printf("In constructor\n");

    // Setup general purpose queue
    _setupGPQueue(DistillerManager::QueueLen);

    // Setup callback table
    _setupCBTable();

    // Check params

    // Init from params
}

PBRet DistillerManager::_setupGPQueue(BaseType_t queueDepth)
{
    _GPQueue = xQueueCreate(queueDepth, sizeof(MessageBase*));

    if (_GPQueue == NULL) {
        ESP_LOGW(DistillerManager::Name, "Unable to create general purpose queue for DistillerManager");
        // Set some flags to indicate system state
        return PBRet::FAILURE;
    }

    ESP_LOGI(DistillerManager::Name, "General purpose queue for DistillerManager successfully created!");
    return PBRet::SUCCESS;
}

void DistillerManager::taskMain(void)
{
    std::set<MessageType> subscriptions = { MessageType::General };
    Subscriber sub(DistillerManager::Name, _GPQueue, subscriptions);
    MessageServer::registerTask(sub);

    // Send a test message to the queue
    GeneralMessage* msg = new GeneralMessage("This is a test");
    MessageServer::broadcastMessage(msg);

    // TODO: Change this task to wake when messages are in the queue
    
    while(1) {
        // Check if a message is in the queue

        _processQueue();
        // UBaseType_t n_messages = uxQueueMessagesWaiting(_GPQueue);
        // if (n_messages > 0) {
        //     // Assume message is general, for now
        //     GeneralMessage* msg;
        //     if (xQueueReceive(_GPQueue, &msg, 10 / portTICK_RATE_MS) == pdTRUE) {
        //         ESP_LOGI(DistillerManager::Name, "Got message: %s", msg->getMessage().c_str());
        //     }
        // }
        vTaskDelay(100 / portTICK_RATE_MS);
    }
}

PBRet DistillerManager::_generalMessagCB(GeneralMessage* msg)
{
    ESP_LOGI(DistillerManager::Name, "Entered GeneralMessage cb in DistillerManager");

    return PBRet::SUCCESS;
}

PBRet DistillerManager::_setupCBTable(void)
{
    _cbTable.insert(std::pair<MessageType, std::function<PBRet(MessageBase*)>>(
        MessageType::General, 
        this->_generalMessagCB
    ));

    // TODO: Indicate success of table insertion
    return PBRet::SUCCESS;
}
