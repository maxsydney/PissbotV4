
#include "WebServer.h"
#include "WebServerMessaging.h"
#include "ConnectionManager.h"
#include "SensorManager.h"
#include "SensorManager.h"
#include "Controller.h"
#include "espfs.h"
#include "espfs_image.h"
#include "libesphttpd/httpd-espfs.h"
#include "esp_netif.h"
#include "libesphttpd/route.h"
#include "IO/Writable.h"
#include "cJSON.h"

Webserver::Webserver(UBaseType_t priority, UBaseType_t stackDepth, BaseType_t coreID, const WebserverConfig& cfg)
    : Task(Webserver::Name, priority, stackDepth, coreID)
{
    // Setup callback table
    _setupCBTable();

    // Init from params
    if (_initFromParams(cfg) == PBRet::SUCCESS) {
        ESP_LOGI(Webserver::Name, "Webserver configured!");
        _configured = true;
    } else {
        ESP_LOGW(Webserver::Name, "Unable to configure Webserver");
    }
}

void Webserver::taskMain(void)
{
    // Subscribe to messages
    std::set<PBMessageType> subscriptions = { 
        PBMessageType::TemperatureData,
        PBMessageType::ControllerTuning,
        PBMessageType::ControllerCommand,
        PBMessageType::ControllerSettings,
        PBMessageType::DeviceData,
        PBMessageType::FlowrateData,
        PBMessageType::ConcentrationData,
        PBMessageType::ControllerState,
        PBMessageType::SocketLog
    };
    Subscriber sub(Webserver::Name, _GPQueue, subscriptions);
    MessageServer::registerTask(sub);

    // Set update frequency
    const TickType_t updatePeriod =  1000 / (_cfg.maxBroadcastFreq * portTICK_PERIOD_MS);
    portTickType xLastWakeTime = xTaskGetTickCount();

    while (true) {
        // Retrieve data from the queue
        _processQueue();

        vTaskDelayUntil(&xLastWakeTime, updatePeriod);
    }
}

PBRet Webserver::_initFromParams(const WebserverConfig& cfg)
{
    if (Webserver::checkInputs(cfg) != PBRet::SUCCESS) {
        return PBRet::FAILURE;
    }

    if (_startupWebserver(cfg) == PBRet::FAILURE) {
        return PBRet::FAILURE;
    }

    _cfg = cfg;
    return PBRet::SUCCESS;
}

PBRet Webserver::checkInputs(const WebserverConfig& cfg)
{
    if (cfg.maxConnections <= 0) {
        ESP_LOGW(Webserver::Name, "Maximum connections must be greater than 0");
        return PBRet::FAILURE;
    }

    if (cfg.maxBroadcastFreq <= 0.0) {
        ESP_LOGW(Webserver::Name, "Maximum broadcast frequency must be greater than 0 Hz (got %f)", cfg.maxBroadcastFreq);
        return PBRet::FAILURE;
    }

    return PBRet::SUCCESS;
}

PBRet Webserver::loadFromJSON(WebserverConfig& cfg, const cJSON* cfgRoot)
{
    // Load WebserverConfig struct from JSON
    if (cfgRoot == nullptr) {
        ESP_LOGW(Webserver::Name, "cfgRoot was null");
        return PBRet::FAILURE;
    }

    // Get maxConnections
    cJSON* maxConNode = cJSON_GetObjectItem(cfgRoot, "maxConnections");
    if (cJSON_IsNumber(maxConNode)) {
        cfg.maxConnections = maxConNode->valueint;
    } else {
        ESP_LOGI(Webserver::Name, "Unable to read maxConnections from JSON");
        return PBRet::FAILURE;
    }

    // Get maxBroadcastFreq
    cJSON* maxCastFreqNode = cJSON_GetObjectItem(cfgRoot, "maxBroadcastFreq");
    if (cJSON_IsNumber(maxCastFreqNode)) {
        cfg.maxBroadcastFreq = maxCastFreqNode->valueint;
    } else {
        ESP_LOGI(Webserver::Name, "Unable to read maxBroadcastFreq from JSON");
        return PBRet::FAILURE;
    }    

    return PBRet::SUCCESS;
}

void Webserver::openConnection(Websock *ws) 
{
	ESP_LOGI("Webserver", "Got connection request");
    ws->recvCb = Webserver::processWebsocketMessage;
    ws->closeCb = Webserver::closeConnection;
    _requestControllerTuning();
    _requestControllerSettings();
    _requestControllerPeripheralState();
    ConnectionManager::addConnection(ws);
    ConnectionManager::printConnections();
}

void Webserver::closeConnection(Websock *ws)
{
    ConnectionManager::removeConnection(ws);
    ConnectionManager::printConnections();
}

void Webserver::processWebsocketMessage(Websock *ws, char *data, int len, int flags)
{
    // Websocket message is not null terminated. Copy into string with null
    // terminator
    char* socketMsg = (char*) malloc(len+1);
    std::string msgTypeStr {};
    memset(socketMsg, 0, len+1);
    memcpy(socketMsg, data, len);

    ESP_LOGI(Webserver::Name, "Message: %s", socketMsg);

    cJSON* root = cJSON_Parse(socketMsg);
    if (root == nullptr) {
        ESP_LOGW(Webserver::Name, "Failed to parse JSON string");
        return;
    }

    // Get message type
    cJSON* msgType = cJSON_GetObjectItem(root, "type");
    if (msgType != nullptr) {
        msgTypeStr = std::string(msgType->valuestring);
    } else {
        ESP_LOGI(Webserver::Name, "Unable to read msgType from JSON string");
        cJSON_Delete(root);
        return;
    }

    // Parse string
    if (msgTypeStr == Webserver::CtrlTuningStr) {
        _processControlTuningMessage(root);
    } else if (msgTypeStr == Webserver::CtrlSettingsStr) {
        _processControlSettingsMessage(root);
    } else if (msgTypeStr == Webserver::PeripheralState) {
        _processPeripheralStateMessage(root);
    }else if (msgTypeStr == Webserver::CommandStr) {
        _processCommandMessage(root);
    } else {
        ESP_LOGI(Webserver::Name, "Unable to process %s message", msgTypeStr.c_str());
    }

    // Can't return PBRet from this callback
    cJSON_Delete(root);
    return;
}

// TODO: Make this a member of Webserver
static HttpdBuiltInUrl builtInUrls[] = {
	ROUTE_REDIRECT("/", "/index.html"),
    ROUTE_WS("/ws", Webserver::openConnection),
	ROUTE_FILESYSTEM(),
	ROUTE_END()
};

PBRet Webserver::_startupWebserver(const WebserverConfig& cfg)
{
    // Allocate connection memory
    ESP_LOGI(Webserver::Name, "Allocating %d bytes for webserver connection memory", sizeof(RtosConnType) * cfg.maxConnections);
    connectionMemory = (RtosConnType*) malloc(sizeof(RtosConnType) * cfg.maxConnections);
    if (connectionMemory == nullptr) {
        // TODO: In the case of failure, set flag to run without webserver
        ESP_LOGE(Webserver::Name, "Failed to allocate memory for webserver");
        return PBRet::FAILURE;
    }

    // Configure webserver
	EspFsConfig espfs_conf {};
    espfs_conf.memAddr = espfs_image_bin;
	EspFs* fs = espFsInit(&espfs_conf);
    if (fs == nullptr) {
        ESP_LOGE(Webserver::Name, "Failed to initialise espfs file system");
        return PBRet::FAILURE;
    }

    httpdRegisterEspfs(fs);
	httpdFreertosInit(&_httpdFreertosInstance,
	                  builtInUrls,
	                  LISTEN_PORT,
	                  connectionMemory,
	                  cfg.maxConnections,
	                  HTTPD_FLAG_NONE);
	httpdFreertosStart(&_httpdFreertosInstance);

    ESP_LOGI(Webserver::Name, "Webserver initialized");
    return PBRet::SUCCESS;
}

PBRet Webserver::_setupCBTable(void)
{
    _cbTable = std::map<PBMessageType, queueCallback> {
        {PBMessageType::TemperatureData, std::bind(&Webserver::_broadcastDataCB, this, std::placeholders::_1)},
        {PBMessageType::ControllerSettings, std::bind(&Webserver::_broadcastDataCB, this, std::placeholders::_1)},
        {PBMessageType::ControllerTuning, std::bind(&Webserver::_broadcastDataCB, this, std::placeholders::_1)},
        {PBMessageType::DeviceData, std::bind(&Webserver::_broadcastDataCB, this, std::placeholders::_1)},
        {PBMessageType::FlowrateData, std::bind(&Webserver::_broadcastDataCB, this, std::placeholders::_1)},
        {PBMessageType::ControllerCommand, std::bind(&Webserver::_broadcastDataCB, this, std::placeholders::_1)},
        {PBMessageType::ConcentrationData, std::bind(&Webserver::_broadcastDataCB, this, std::placeholders::_1)},
        {PBMessageType::ControllerState, std::bind(&Webserver::_broadcastDataCB, this, std::placeholders::_1)},
        {PBMessageType::SocketLog, std::bind(&Webserver::_broadcastDataCB, this, std::placeholders::_1)}
    };

    return PBRet::SUCCESS;
}

PBRet Webserver::_broadcastDataCB(std::shared_ptr<PBMessageWrapper> msg)
{
    // Broadcasts the wrapped message to all websocket connections

    return _sendToAll(*msg);  
}

PBRet Webserver::_processControlTuningMessage(cJSON* msgRoot)
{
    // ControlTuning ctrlTuning {};
    // if (msgRoot == nullptr) {
    //     ESP_LOGW(Webserver::Name, "Unable to parse control tuning message. cJSON object was null");
    //     return PBRet::FAILURE;
    // }

    // cJSON* msgData = cJSON_GetObjectItemCaseSensitive(msgRoot, "data");
    // if (msgData == nullptr) {
    //     ESP_LOGW(Webserver::Name, "Unable to parse data from control tuning message");
    //     return PBRet::FAILURE;
    // }

    // if (ctrlTuning.deserialize(msgData) != PBRet::SUCCESS) {
    //     ESP_LOGW(Webserver::Name, "Unable to parse control tuning message");
    //     return PBRet::FAILURE;
    // }

    // std::shared_ptr<ControlTuning> msg = std::make_shared<ControlTuning> (ctrlTuning);

    // ESP_LOGI(Webserver::Name, "Broadcasting controller tuning");
    // // return MessageServer::broadcastMessage(msg);

    return PBRet::SUCCESS;
}

PBRet Webserver::_processControlSettingsMessage(cJSON* msgRoot)
{
    // ControlSettings ctrlSettingsMsg {};
    // if (msgRoot == nullptr) {
    //     ESP_LOGW(Webserver::Name, "Unable to parse control settings message. cJSON root object was null");
    //     return PBRet::FAILURE;
    // }

    // cJSON* msgData = cJSON_GetObjectItemCaseSensitive(msgRoot, "data");
    // if (msgData == nullptr) {
    //     ESP_LOGW(Webserver::Name, "Unable to parse data from control settings message");
    //     return PBRet::FAILURE;
    // }

    // if (ctrlSettingsMsg.deserialize(msgData) != PBRet::SUCCESS) {
    //     ESP_LOGW(Webserver::Name, "Unable to parse control settings message. cJSON data object was null");
    //     return PBRet::FAILURE;
    // }

    // std::shared_ptr<ControlSettings> msg = std::make_shared<ControlSettings> (ctrlSettingsMsg);

    // ESP_LOGI(Webserver::Name, "Broadcasting controller settings");
    // // return MessageServer::broadcastMessage(msg);

    return PBRet::SUCCESS;
}

PBRet Webserver::_processCommandMessage(cJSON* msgRoot)
{
    // if (msgRoot == nullptr) {
    //     ESP_LOGW(Webserver::Name, "Unable to parse command message. cJSON object was null");
    //     return PBRet::FAILURE;
    // }

    // std::string subtypeStr {};

    // cJSON* subtype = cJSON_GetObjectItemCaseSensitive(msgRoot, "subtype");
    // if (subtype != nullptr) {
    //     subtypeStr = std::string(subtype->valuestring);
    // } else {
    //     ESP_LOGW(Webserver::Name, "Unable to parse subtype from command message");
    //     return PBRet::FAILURE;
    // }

    // if (subtypeStr == Webserver::BroadcastDevices) {
    //     // Send message to trigger scan of sensor bus
    //     std::shared_ptr<SensorManagerCommand> msg = std::make_shared<SensorManagerCommand> (SensorManagerCmdType::BroadcastSensorsStart);
    //     ESP_LOGI(Webserver::Name, "Sending message to start sensor assign task");
    //     // return MessageServer::broadcastMessage(msg);

    //     return PBRet::SUCCESS;
    // } else if (subtypeStr == Webserver::AssignSensor) {
    //     return _processAssignSensorMessage(msgRoot);
    // } else {
    //     ESP_LOGW(Webserver::Name, "Unable to parse command message with subtype %s", subtypeStr.c_str());
    //     return PBRet::FAILURE;
    // }

    return PBRet::SUCCESS;
}

PBRet Webserver::_processPeripheralStateMessage(cJSON* msgRoot)
{
    // ControlCommand ctrlCommand {};
    // if (msgRoot == nullptr) {
    //     ESP_LOGW(Webserver::Name, "Unable to parse control command message. cJSON object was null");
    //     return PBRet::FAILURE;
    // }

    // cJSON* msgData = cJSON_GetObjectItemCaseSensitive(msgRoot, "data");
    // if (msgData == nullptr) {
    //     ESP_LOGW(Webserver::Name, "Unable to parse data from control command message");
    //     return PBRet::FAILURE;
    // }

    // if (ctrlCommand.deserialize(msgData) != PBRet::SUCCESS) {
    //     ESP_LOGW(Webserver::Name, "Unable to parse control command message");
    //     return PBRet::FAILURE;
    // }

    // // std::shared_ptr<ControlCommand> msg = std::make_shared<ControlCommand> (ctrlCommand);

    // ESP_LOGI(Webserver::Name, "Broadcasting controller command");
    // // return MessageServer::broadcastMessage(msg);

    return PBRet::SUCCESS;
}

PBRet Webserver::_processAssignSensorMessage(cJSON* root)
{
//     if (root == nullptr) {
//         ESP_LOGW(Webserver::Name, "Unable to parse assign sensor message. cJSON object was null");
//         return PBRet::FAILURE;
//     }

//     ESP_LOGI(Webserver::Name, "Decoding assign sensor message");

//     cJSON* sensorData = cJSON_GetObjectItemCaseSensitive(root, "data");
//     if (sensorData == nullptr) {
//         ESP_LOGW(Webserver::Name, "Unable to parse sensor data from assign sensor message");
//         return PBRet::FAILURE;
//     }

//     // TODO: Allow Ds18b20 initialization from JSON
//     cJSON* sensorAddr = cJSON_GetObjectItemCaseSensitive(sensorData, "addr");
//     if (sensorAddr == nullptr) {
//         ESP_LOGW(Webserver::Name, "Unable to parse sensor address from assign sensor message");
//         return PBRet::FAILURE;
//     }

//     // Read address from JSON
//     OneWireBus_ROMCode romCode {};
//     cJSON* byte = nullptr;
//     int i = 0;
//     cJSON_ArrayForEach(byte, sensorAddr)
//     {
//         if (cJSON_IsNumber(byte) == false) {
//             ESP_LOGW(Webserver::Name, "Unable to decode sensor address. Address byte was not a number");
//             return PBRet::FAILURE;
//         }
//         romCode.bytes[i++] = byte->valueint;
//     }

//     // Read assigned sensor task from JSON
//     cJSON* task = cJSON_GetObjectItemCaseSensitive(sensorData, "task");
//     if (task == nullptr) {
//         ESP_LOGW(Webserver::Name, "Unable to parse assigned sensor task from assign sensor message");
//         return PBRet::FAILURE;
//     }
//     SensorType sensorType = PBOneWire::mapSensorIDToType(task->valueint);

//     std::shared_ptr<AssignSensorCommand> msg = std::make_shared<AssignSensorCommand> (romCode, sensorType);
//     // return MessageServer::broadcastMessage(msg);

    return PBRet::SUCCESS;
}

PBRet Webserver::_sendToAll(const std::string& msg)
{
    // Send a message to all open websocket connections
    for (Websock* ws : ConnectionManager::getActiveWebsockets()) {
        if (msg.length() > 0) {
            int ret = cgiWebsocketSend(&_httpdFreertosInstance.httpdInstance, ws, msg.c_str(), strlen(msg.c_str()), WEBSOCK_FLAG_NONE);
            if (ret != 1) {
                ESP_LOGW(Webserver::Name, "Unable to send message to websocket %p (got %d)", ws, ret);
            }
        }
    }

    return PBRet::SUCCESS;
}

PBRet Webserver::_sendToAll(const PBMessageWrapper& wrapper)
{
    // Serialize to byte buffer
    Writable buffer {};
    wrapper.serialize(buffer);      // TODO: Error checking

    // Send a message to all open websocket connections
    for (Websock* ws : ConnectionManager::getActiveWebsockets()) {
        if (buffer.get_size() > 0) {
            int ret = cgiWebsocketSend(&_httpdFreertosInstance.httpdInstance, ws, reinterpret_cast<const char*>(buffer.get_buffer()), 
                                       buffer.get_size(), WEBSOCK_FLAG_BIN);
            if (ret != 1) {
                ESP_LOGW(Webserver::Name, "Unable to send message to websocket %p (got %d)", ws, ret);
            }
        }
    }

    return PBRet::SUCCESS;
}

PBRet Webserver::_requestControllerTuning(void)
{
    // Request controller tuning settings
    // 

    ControllerDataRequest request {};
    request.set_requestType(ControllerDataRequestType::TUNING);

    PBMessageWrapper wrapped = MessageServer::wrap(request, PBMessageType::ControllerDataRequest);
    std::shared_ptr<PBMessageWrapper> msg = std::make_shared<PBMessageWrapper> (wrapped);

    ESP_LOGI(Webserver::Name, "Requesting controller tuning");

    return MessageServer::broadcastMessage(msg);
}

PBRet Webserver::_requestControllerSettings(void)
{
    // Request controller settings
    // 

    ControllerDataRequest request {};
    request.set_requestType(ControllerDataRequestType::SETTINGS);

    PBMessageWrapper wrapped = MessageServer::wrap(request, PBMessageType::ControllerDataRequest);
    std::shared_ptr<PBMessageWrapper> msg = std::make_shared<PBMessageWrapper> (wrapped);

    return MessageServer::broadcastMessage(msg);
}

PBRet Webserver::_requestControllerPeripheralState(void)
{
    // Request controller settings
    // 
    ControllerDataRequest request {};
    request.set_requestType(ControllerDataRequestType::PERIPHERAL_STATE);

    PBMessageWrapper wrapped = MessageServer::wrap(request, PBMessageType::ControllerDataRequest);
    std::shared_ptr<PBMessageWrapper> msg = std::make_shared<PBMessageWrapper> (wrapped);

    return MessageServer::broadcastMessage(msg);
}

PBRet Webserver::socketLog(const std::string& logMsg)
{
    // Broadcast a logged message to all available sockets
    PBSocketLogMessage socketLog {};
    socketLog.mutable_logMsg().set(logMsg.c_str(), logMsg.length());
    PBMessageWrapper wrapped = MessageServer::wrap(socketLog, PBMessageType::SocketLog);
    std::shared_ptr<PBMessageWrapper> msgPtr = std::make_shared<PBMessageWrapper> (wrapped);
    
    return MessageServer::broadcastMessage(msgPtr);
}

