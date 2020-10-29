#include <functional>
#include "DistillerManager.h"
#include "WifiManager.h"
#include "messageServer.h"
#include "MessageDefs.h"
#include "nvs_flash.h"

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

    // Init from params
    if (_initFromParams(cfg) == PBRet::SUCCESS) {
        ESP_LOGI(DistillerManager::Name, "DistillerManager configured!");
        _configured = true;
    } else {
        ESP_LOGW(DistillerManager::Name, "Unable to configure DistillerManager");
    }
}

void DistillerManager::taskMain(void)
{
    std::set<MessageType> subscriptions = { MessageType::General };
    Subscriber sub(DistillerManager::Name, _GPQueue, subscriptions);
    MessageServer::registerTask(sub);

    // Send a test message to the queue
    std::shared_ptr<GeneralMessage> msg = std::make_shared<GeneralMessage> ("This is a test");
    MessageServer::broadcastMessage(msg);

    while(true) {
        _processQueue();

        vTaskDelay(100 / portTICK_RATE_MS);
    }
}

PBRet DistillerManager::_generalMessagCB(std::shared_ptr<MessageBase> msg)
{
    std::shared_ptr<GeneralMessage> genMsg = std::static_pointer_cast<GeneralMessage>(msg);
    ESP_LOGI(DistillerManager::Name, "Received general message: %s", genMsg->getMessage().c_str());  

    return PBRet::SUCCESS;
}

PBRet DistillerManager::_setupCBTable(void)
{
    _cbTable = std::map<MessageType, queueCallback> {
        {MessageType::General, std::bind(&DistillerManager::_generalMessagCB, this, std::placeholders::_1)}
    };

    return PBRet::SUCCESS;
}

PBRet DistillerManager::_initFromParams(const DistillerConfig& cfg)
{
    if (DistillerManager::checkInputs(cfg) != PBRet::SUCCESS) {;
        ESP_LOGW(DistillerManager::Name, "Unable to configure DistillerManager");
        return PBRet::FAILURE;
    }

    // Connect to Wifi
    WifiManager::connect("PBLink", "pissbot1");

    // Initialize Controller
    _controller = std::make_unique<Controller> (7, 8192, 1, cfg.ctrlConfig);
    if (_controller->isConfigured()) {
        _controller->begin();
    } else {
        ESP_LOGW(DistillerManager::Name, "Unable to start controller");
    }

    // Initialize SensorManager
    _sensorManager = std::make_unique<SensorManager> (7, 8192, 0, cfg.sensorManagerConfig);
    if (_sensorManager->isConfigured()) {
        _sensorManager->begin();
    } else {
        ESP_LOGW(DistillerManager::Name, "Unable to start sensor manager");
    }

    // Initialize Webserver
    ESP_LOGI(DistillerManager::Name, "Initializing Webserver");
    _webserver = std::make_unique<Webserver> (cfg.webserverConfig);
    if (_webserver->isConfigured() == false) {
        ESP_LOGW(DistillerManager::Name, "Unable to start sensor webserver");
    }

    return PBRet::SUCCESS;
}

PBRet DistillerManager::checkInputs(const DistillerConfig& cfg)
{
    // TODO: Implement this

    return PBRet::SUCCESS;
}
