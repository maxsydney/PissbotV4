#include <functional>
#include "DistillerManager.h"
#include "WifiManager.h"
#include "MessageServer.h"
#include "MessageDefs.h"
#include "nvs_flash.h"
#include "Generated/WebserverMessaging.h"
#include "IO/Readable.h"

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
    std::set<PBMessageType> subscriptions = { PBMessageType::SocketLog };
    Subscriber sub(DistillerManager::Name, _GPQueue, subscriptions);
    MessageServer::registerTask(sub);

    // Create a SocketLog message and broadcast it
    std::string testMessage("This is a test");
    PBSocketLogMessage msg {};
    msg.mutable_logMsg().set(testMessage.c_str(), testMessage.length());
    PBMessageWrapper wrapped = MessageServer::wrapMessage(msg, PBMessageType::SocketLog);
    std::shared_ptr<PBMessageWrapper> msgPtr = std::make_shared<PBMessageWrapper> (wrapped);
    MessageServer::broadcastMessage(msgPtr);
    ESP_LOGI(DistillerManager::Name, "Broadcast test socket log message!");


    while(true) {
        _processQueue();

        _doHeartbeat();

        vTaskDelay(100 / portTICK_RATE_MS);
    }
}

PBRet DistillerManager::_socketLogCB(std::shared_ptr<PBMessageWrapper> msg)
{
    ESP_LOGI(DistillerManager::Name, "Received socket log message");
    Readable readBuffer {};
    for (size_t i = 0; i < msg->get_payload().get_length(); i++)
    {
        uint8_t ch = static_cast<uint8_t>(msg->get_payload().get_const(i));
        readBuffer.push(ch);
    }

    PBSocketLogMessage decodedMsg {};
    decodedMsg.deserialize(readBuffer);

    ESP_LOGI(DistillerManager::Name, "Message: %s", decodedMsg.logMsg());

    return PBRet::SUCCESS;
}

PBRet DistillerManager::_setupCBTable(void)
{
    _cbTable = std::map<PBMessageType, queueCallback> {
        {PBMessageType::SocketLog, std::bind(&DistillerManager::_socketLogCB, this, std::placeholders::_1)}
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
    _webserver = std::make_unique<Webserver> (7, 8192, 1, cfg.webserverConfig);
    if (_webserver->isConfigured()) {
        _webserver->begin();
    } else {
        ESP_LOGW(DistillerManager::Name, "Unable to start sensor webserver");
    }

    // Initialize LED GPIO
    _LEDGPIO = cfg.LEDGPIO;
    PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[_LEDGPIO], PIN_FUNC_GPIO);
    gpio_set_direction(_LEDGPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(_LEDGPIO, 0); 

    return PBRet::SUCCESS;
}

PBRet DistillerManager::checkInputs(const DistillerConfig& cfg)
{
    // TODO: Implement this

    return PBRet::SUCCESS;
}

PBRet DistillerManager::loadFromJSON(DistillerConfig& cfg, const cJSON* cfgRoot)
{
    // Load DistillerConfig struct from JSON

    if (cfgRoot == nullptr) {
        ESP_LOGW(DistillerManager::Name, "cfg was null");
        return PBRet::FAILURE;
    }

    // Load controller configuration
    cJSON* controllerNode = cJSON_GetObjectItem(cfgRoot, "ControllerConfig");
    if (Controller::loadFromJSON(cfg.ctrlConfig, controllerNode) != PBRet::SUCCESS) {
        return PBRet::FAILURE;
    }

    // Load SensorManager configuration
    cJSON* sensorManagerNode = cJSON_GetObjectItem(cfgRoot, "SensorManagerConfig");
    if (SensorManager::loadFromJSON(cfg.sensorManagerConfig, sensorManagerNode) != PBRet::SUCCESS) {
        return PBRet::FAILURE;
    }

    // Load Webserver configuration
    cJSON* webserverNode = cJSON_GetObjectItem(cfgRoot, "WebserverConfig");
    if (Webserver::loadFromJSON(cfg.webserverConfig, webserverNode) != PBRet::SUCCESS) {
        return PBRet::FAILURE;
    }

    // Load LED pin
    cJSON* LEDGPIONode = cJSON_GetObjectItem(cfgRoot, "LEDGPIO");
    if (cJSON_IsNumber(LEDGPIONode)) {
        cfg.LEDGPIO = static_cast<gpio_num_t>(LEDGPIONode->valueint);
    } else {
        ESP_LOGW(DistillerManager::Name, "Unable to read LED GPIO from JSON");
        return PBRet::FAILURE;
    }
    
    return PBRet::SUCCESS;
}

PBRet DistillerManager::_doHeartbeat(void)
{
    // Periodically transmit heartbeat signal
    const double timeSinceLastHeartbeat = esp_timer_get_time() * 1e-3 - _lastHeartbeatTime;

    if (timeSinceLastHeartbeat > DistillerManager::HeartBeatPeriod) {
        ESP_LOGI(DistillerManager::Name, "Heartbeat");

        gpio_set_level(_LEDGPIO, 1);
        vTaskDelay(100 / portTICK_PERIOD_MS);
        gpio_set_level(_LEDGPIO, 0);

        _lastHeartbeatTime = esp_timer_get_time() * 1e-3;
    }

    return PBRet::SUCCESS;
}