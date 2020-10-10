#include <functional>
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
    : Task(DistillerManager::Name, priority, stackDepth, coreID)
{
    // Setup callback table
    _setupCBTable();

    // Check params

    // Init from params

    ESP_LOGI(DistillerManager::Name, "Distiller Manager initialized successfully");
}

void DistillerManager::taskMain(void)
{
    std::set<MessageType> subscriptions = { MessageType::General };
    Subscriber sub(DistillerManager::Name, _GPQueue, subscriptions);
    MessageServer::registerTask(sub);

    // // Send a test message to the queue
    std::shared_ptr<GeneralMessage> msg = std::make_shared<GeneralMessage> ("This is a test");
    MessageServer::broadcastMessage(msg);

    while(1) {
        _processQueue();

        vTaskDelay(100 / portTICK_RATE_MS);
    }
}

PBRet DistillerManager::_generalMessagCB(std::shared_ptr<MessageBase> msg)
{
    std::shared_ptr<GeneralMessage> genMsg = std::static_pointer_cast<GeneralMessage>(msg);
    ESP_LOGI(DistillerManager::Name, "Entered GeneralMessage cb in DistillerManager");
    ESP_LOGI(DistillerManager::Name, "Message was: %s", genMsg->getMessage().c_str());  

    return PBRet::SUCCESS;
}

PBRet DistillerManager::_setupCBTable(void)
{
    _cbTable = std::map<MessageType, queueCallback> {
        {MessageType::General, std::bind(&DistillerManager::_generalMessagCB, this, std::placeholders::_1)}
    };

    return PBRet::SUCCESS;
}
