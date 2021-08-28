
#include "WebServer.h"
#include "WebServerMessaging.h"
#include "ConnectionManager.h"
#include "SensorManager.h"
#include "SensorManager.h"
#include "Controller.h"
#include "libespfs/espfs.h"
#include "libesphttpd/httpd-espfs.h"
#include "esp_netif.h"
#include "libesphttpd/route.h"
#include "IO/Readable.h"
#include "IO/Writable.h"
#include "cJSON.h"

extern const uint8_t espfs_bin[];

Webserver::Webserver(UBaseType_t priority, UBaseType_t stackDepth, BaseType_t coreID, const WebserverConfig& cfg)
    : Task(Webserver::Name, priority, stackDepth, coreID)
{
    // Setup callback table
    _setupCBTable();

    // Set message ID
    Task::_ID = MessageOrigin::Webserver;

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
    // Read in message from websocket and broadcast over network

    Readable readBuffer {};
    for (size_t i = 0; i < len; i++)
    {
        readBuffer.push(static_cast<uint8_t>(data[i]));
    }

    PBMessageWrapper wrapped{};
    EmbeddedProto::Error err = wrapped.deserialize(readBuffer);

    if (err != EmbeddedProto::Error::NO_ERRORS) {
        ESP_LOGW(Webserver::Name, "Failed to decode message");
        MessageServer::printErr(err);
        return;
    }

    MessageServer::broadcastMessage(wrapped);
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
	espfs_config_t espfs_conf {};
    espfs_conf.addr = espfs_bin;
	espfs_fs_t* fs = espfs_init(&espfs_conf);
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

    PBMessageWrapper wrapped = MessageServer::wrap(request, PBMessageType::ControllerDataRequest, MessageOrigin::Webserver);
    ESP_LOGI(Webserver::Name, "Requesting controller tuning");

    return MessageServer::broadcastMessage(wrapped);
}

PBRet Webserver::_requestControllerSettings(void)
{
    // Request controller settings
    // 

    ControllerDataRequest request {};
    request.set_requestType(ControllerDataRequestType::SETTINGS);
    PBMessageWrapper wrapped = MessageServer::wrap(request, PBMessageType::ControllerDataRequest, MessageOrigin::Webserver);

    return MessageServer::broadcastMessage(wrapped);
}

PBRet Webserver::_requestControllerPeripheralState(void)
{
    // Request controller settings
    // 
    ControllerDataRequest request {};
    request.set_requestType(ControllerDataRequestType::PERIPHERAL_STATE);
    PBMessageWrapper wrapped = MessageServer::wrap(request, PBMessageType::ControllerDataRequest, MessageOrigin::Webserver);

    return MessageServer::broadcastMessage(wrapped);
}

PBRet Webserver::socketLog(const std::string& logMsg)
{
    // Broadcast a logged message to all available sockets
    PBSocketLogMessage socketLog {};
    socketLog.mutable_logMsg().set(logMsg.c_str(), logMsg.length());
    PBMessageWrapper wrapped = MessageServer::wrap(socketLog, PBMessageType::SocketLog, MessageOrigin::Log);
    
    return MessageServer::broadcastMessage(wrapped);
}

