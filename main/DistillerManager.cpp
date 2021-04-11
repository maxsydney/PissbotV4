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

    while(true) {
        _processQueue();

        _doHeartbeat();

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