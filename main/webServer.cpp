
#include "Webserver.h"
#include "connectionManager.h"
#include "controller.h"
#include "espfs.h"
#include "espfs_image.h"
#include "libesphttpd/httpd-espfs.h"
#include "esp_netif.h"
#include "libesphttpd/route.h"
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
    std::set<MessageType> subscriptions = { 
        MessageType::TemperatureData,
        MessageType::ControlTuning,
        MessageType::ControlCommand,
        MessageType::ControlSettings,
        MessageType::DeviceData
    };
    Subscriber sub(Webserver::Name, _GPQueue, subscriptions);
    MessageServer::registerTask(sub);

    // Set update frequency
    const TickType_t timestep =  1000 / (_cfg.maxBroadcastFreq * portTICK_PERIOD_MS);
    portTickType xLastWakeTime = xTaskGetTickCount();

    while (true) {
        // Retrieve data from the queue
        _processQueue();

        // Broadcast data
        _sendToAll(_temperatureMessage);
        _sendToAll(_ctrlTuningMessage);
        _sendToAll(_ctrlSettingsMessage);

        // Clear sent messages
        _temperatureMessage.clear();
        _ctrlTuningMessage.clear();
        _ctrlSettingsMessage.clear();

        vTaskDelayUntil(&xLastWakeTime, timestep);
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

void Webserver::openConnection(Websock *ws) 
{
	ESP_LOGI("Webserver", "Got connection request");
    ws->recvCb = Webserver::processWebsocketMessage;
    ws->closeCb = Webserver::closeConnection;
    _requestControllerTuning();
    _requestControllerSettings();
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
        _parseControlTuningMessage(root);
    } else if (msgTypeStr == Webserver::CtrlSettingsStr) {
        _parseControlSettingsMessage(root);
    } else if (msgTypeStr == Webserver::CommandStr) {
        _parseCommandMessage(root);
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
    _cbTable = std::map<MessageType, queueCallback> {
        {MessageType::TemperatureData, std::bind(&Webserver::_temperatureDataCB, this, std::placeholders::_1)},
        {MessageType::ControlSettings, std::bind(&Webserver::_controlSettingsCB, this, std::placeholders::_1)},
        {MessageType::ControlTuning, std::bind(&Webserver::_controlTuningCB, this, std::placeholders::_1)},
        {MessageType::DeviceData, std::bind(&Webserver::_deviceDataCB, this, std::placeholders::_1)}        
    };

    return PBRet::SUCCESS;
}

PBRet Webserver::_temperatureDataCB(std::shared_ptr<MessageBase> msg)
{
    // Take the temperature data and broadcast it to all available
    // websocket connections. To avoid spamming the network, if several
    // temperature data messages are in the queue, only the most recent
    // is sent. 
    //
    // Temperature data is serialized to JSON and broadcast over websockets
    // to the browser client

    // Get TemperatureData object
    TemperatureData TData = *std::static_pointer_cast<TemperatureData>(msg);

    // Serialize to TemperatureData JSON string memory
    if (serializeTemperatureDataMsg(TData, _temperatureMessage) != PBRet::SUCCESS)
    {
        ESP_LOGW(Webserver::Name, "Error writing TemperatureData object to JSON string. Deleting");
        _temperatureMessage.clear();
        return PBRet::FAILURE;
    }

    return PBRet::SUCCESS;   
}

PBRet Webserver::_controlSettingsCB(std::shared_ptr<MessageBase> msg)
{
    // Serialize the controller settings and broadcast to all 
    // connected websockets

    // Get controlTuning object
    ControlSettings ctrlSettings = *std::static_pointer_cast<ControlSettings>(msg);

    // Serialize to ControlTuning JSON string memory
    if (serializeControlSettingsMessage(ctrlSettings, _ctrlSettingsMessage) != PBRet::SUCCESS)
    {
        ESP_LOGW(Webserver::Name, "Error writing ControlSettings object to JSON string. Deleting");
        _ctrlSettingsMessage.clear();
        return PBRet::FAILURE;
    }

    return PBRet::SUCCESS;
}

PBRet Webserver::_controlTuningCB(std::shared_ptr<MessageBase> msg)
{
    // Serialize the controller tuning parameters and broadcast to all 
    // connected websockets

    // Get controlTuning object
    ControlTuning ctrlTuning = *std::static_pointer_cast<ControlTuning>(msg);

    // Serialize to ControlTuning JSON string memory
    if (serializeControlTuningMsg(ctrlTuning, _ctrlTuningMessage) != PBRet::SUCCESS)
    {
        ESP_LOGW(Webserver::Name, "Error writing ControlTuning object to JSON string. Deleting");
        _ctrlTuningMessage.clear();
        return PBRet::FAILURE;
    }

    return PBRet::SUCCESS;
}

PBRet Webserver::_deviceDataCB(std::shared_ptr<MessageBase> msg)
{
    // Serialize the device data message and broadcast to all connected
    // websockets

    // Get controlTuning object
    DeviceData deviceData = *std::static_pointer_cast<DeviceData>(msg);

    // Serialize the device addresses
    cJSON* root = cJSON_CreateObject();
    if (root == nullptr) {
        ESP_LOGW(Webserver::Name, "Unable to create root JSON object");
        return PBRet::FAILURE;
    }

    cJSON* sensors = cJSON_CreateArray();
    if (sensors == nullptr) {
        ESP_LOGW(Webserver::Name, "Unable to create sensors array");
        cJSON_Delete(root);
        return PBRet::FAILURE;
    }

    cJSON_AddStringToObject(root, "type", "sensorID");
    cJSON_AddItemToObject(root, "sensors", sensors);

    for (const Ds18b20& sensor: deviceData.getDevices()) {
        sensor.serialize(sensors);
    }

    char* deviceDataJSON = cJSON_Print(root);
    cJSON_Delete(root);

    if (deviceDataJSON == nullptr) {
        ESP_LOGW(Webserver::Name, "Unable to allocate memory for device data JSON string");
        return PBRet::FAILURE;
    }

    _sendToAll(deviceDataJSON);
    free(deviceDataJSON);

    return PBRet::SUCCESS;
}

PBRet Webserver::serializeTemperatureDataMsg(const TemperatureData& TData, std::string& outStr)
{
    // Convert a TemperatureData message into a JSON representation to send
    // to browser client
    cJSON* root = cJSON_CreateObject();
    if (root == nullptr) {
        ESP_LOGW(Webserver::Name, "Unable to create root JSON object");
        return PBRet::FAILURE;
    }

    if (cJSON_AddStringToObject(root, "MessageType", "Temperature") == nullptr)
    {
        ESP_LOGW(Webserver::Name, "Unable to add MessageType to temperature JSON string");
        cJSON_Delete(root);
        return PBRet::FAILURE;
    }

    if (cJSON_AddNumberToObject(root, "HeadTemp", TData.getHeadTemp()) == nullptr) {
        ESP_LOGW(Webserver::Name, "Unable to add head temp to temperature JSON string");
        cJSON_Delete(root);
        return PBRet::FAILURE;
    }

    if (cJSON_AddNumberToObject(root, "RefluxTemp", TData.getRefluxCondensorTemp()) == nullptr) {
        ESP_LOGW(Webserver::Name, "Unable to add reflux condensor temp to temperature JSON string");
        cJSON_Delete(root);
        return PBRet::FAILURE;
    }

    if (cJSON_AddNumberToObject(root, "ProdTemp", TData.getProdCondensorTemp()) == nullptr) {
        ESP_LOGW(Webserver::Name, "Unable to add product condensor temp to temperature JSON string");
        cJSON_Delete(root);
        return PBRet::FAILURE;
    }

    if (cJSON_AddNumberToObject(root, "RadiatorTemp", TData.getRadiatorTemp()) == nullptr) {
        ESP_LOGW(Webserver::Name, "Unable to add radiator temp to temperature JSON string");
        cJSON_Delete(root);
        return PBRet::FAILURE;
    }

    if (cJSON_AddNumberToObject(root, "BoilerTemp", TData.getBoilerTemp()) == nullptr) {
        ESP_LOGW(Webserver::Name, "Unable to add boiler temp to temperature JSON string");
        cJSON_Delete(root);
        return PBRet::FAILURE;
    }

    if (cJSON_AddNumberToObject(root, "uptime", TData.getTimeStamp()) == nullptr) {
        ESP_LOGW(Webserver::Name, "Unable to add timestamp to temperature JSON string");
        cJSON_Delete(root);
        return PBRet::FAILURE;
    }

    char* tempMessageJSON = cJSON_Print(root);
    cJSON_Delete(root);

    if (tempMessageJSON == nullptr) {
        ESP_LOGW(Webserver::Name, "Unable to allocate memory for temperature data JSON string");
        return PBRet::FAILURE;
    }

    outStr = tempMessageJSON;
    free(tempMessageJSON);

    return PBRet::SUCCESS;
}

PBRet Webserver::serializeControlTuningMsg(const ControlTuning& ctrlTuning, std::string& outStr)
{
    // Convert a ControlTuning message into a JSON representation
    //

    cJSON* root = cJSON_CreateObject();
    if (root == nullptr) {
        ESP_LOGW(Webserver::Name, "Unable to create root JSON object");
        return PBRet::FAILURE;
    }

    if (cJSON_AddStringToObject(root, "MessageType", "ControlTuning") == nullptr)
    {
        ESP_LOGW(Webserver::Name, "Unable to add MessageType to ControlTuning JSON string");
        cJSON_Delete(root);
        return PBRet::FAILURE;
    }

    if (cJSON_AddNumberToObject(root, "Setpoint", ctrlTuning.getSetpoint()) == nullptr) {
        ESP_LOGW(Webserver::Name, "Unable to add setpoint to control tuning JSON string");
        cJSON_Delete(root);
        return PBRet::FAILURE;
    }

    if (cJSON_AddNumberToObject(root, "PGain", ctrlTuning.getPGain()) == nullptr) {
        ESP_LOGW(Webserver::Name, "Unable to add P gain to control tuning JSON string");
        cJSON_Delete(root);
        return PBRet::FAILURE;
    }

    if (cJSON_AddNumberToObject(root, "IGain", ctrlTuning.getIGain()) == nullptr) {
        ESP_LOGW(Webserver::Name, "Unable to add I gain to control tuning JSON string");
        cJSON_Delete(root);
        return PBRet::FAILURE;
    }

    if (cJSON_AddNumberToObject(root, "DGain", ctrlTuning.getDGain()) == nullptr) {
        ESP_LOGW(Webserver::Name, "Unable to add D gain to control tuning JSON string");
        cJSON_Delete(root);
        return PBRet::FAILURE;
    }

    if (cJSON_AddNumberToObject(root, "LPFCutoff", ctrlTuning.getLPFCutoff()) == nullptr) {
        ESP_LOGW(Webserver::Name, "Unable to add LPF cutoff to control tuning JSON string");
        cJSON_Delete(root);
        return PBRet::FAILURE;
    }

    char* tuningMsgJson = cJSON_Print(root);
    cJSON_Delete(root);

    if (tuningMsgJson == nullptr) {
        ESP_LOGW(Webserver::Name, "Unable to allocate memory for control tuning data JSON string");
        return PBRet::FAILURE;
    }

    outStr = tuningMsgJson;
    free(tuningMsgJson);

    return PBRet::SUCCESS;
}

PBRet Webserver::serializeControlSettingsMessage(const ControlSettings& ctrlSettings, std::string& outStr)
{
    // Convert a ControlTuning message into a JSON representation
    //

    cJSON* root = cJSON_CreateObject();
    if (root == nullptr) {
        ESP_LOGW(Webserver::Name, "Unable to create root JSON object");
        return PBRet::FAILURE;
    }

    if (cJSON_AddStringToObject(root, "MessageType", "ControlSettings") == nullptr)
    {
        ESP_LOGW(Webserver::Name, "Unable to add MessageType to control settings JSON string");
        cJSON_Delete(root);
        return PBRet::FAILURE;
    }

    if (cJSON_AddNumberToObject(root, "FanState", ctrlSettings.getFanState()) == nullptr) {
        ESP_LOGW(Webserver::Name, "Unable to add fanstate to control settings JSON string");
        cJSON_Delete(root);
        return PBRet::FAILURE;
    }

    if (cJSON_AddNumberToObject(root, "ElementLow", ctrlSettings.getElementLow()) == nullptr) {
        ESP_LOGW(Webserver::Name, "Unable to add low power element to settings tuning JSON string");
        cJSON_Delete(root);
        return PBRet::FAILURE;
    } 

    if (cJSON_AddNumberToObject(root, "ElementHigh", ctrlSettings.getElementHigh()) == nullptr) {
        ESP_LOGW(Webserver::Name, "Unable to add high power element to control settings JSON string");
        cJSON_Delete(root);
        return PBRet::FAILURE;
    } 

    if (cJSON_AddNumberToObject(root, "ProdPump", ctrlSettings.getProdCondensorPump()) == nullptr) {
        ESP_LOGW(Webserver::Name, "Unable to add product condensor pump state to control settings JSON string");
        cJSON_Delete(root);
        return PBRet::FAILURE;
    }

    if (cJSON_AddNumberToObject(root, "RefluxPump", ctrlSettings.getRefluxCondensorPump()) == nullptr) {
        ESP_LOGW(Webserver::Name, "Unable to add reflux condensor pump state to control settings JSON string");
        cJSON_Delete(root);
        return PBRet::FAILURE;
    }

    char* ctrlSettingsMsgJson = cJSON_Print(root);
    cJSON_Delete(root);

    if (ctrlSettingsMsgJson == nullptr) {
        ESP_LOGW(Webserver::Name, "Unable to allocate memory for control settings data JSON string");
        return PBRet::FAILURE;
    }

    outStr = ctrlSettingsMsgJson;
    free(ctrlSettingsMsgJson);

    return PBRet::SUCCESS;
}

PBRet Webserver::_parseControlTuningMessage(cJSON* msgRoot)
{
    ControlTuning ctrlTuning {};
    double PGain, IGain, DGain, setpoint, LPFCutoff;

    if (msgRoot == nullptr) {
        ESP_LOGW(Webserver::Name, "Unable to parse control tuning message. cJSON object was null");
        return PBRet::FAILURE;
    }

    cJSON* msgData = cJSON_GetObjectItemCaseSensitive(msgRoot, "data");
    if (msgData == nullptr) {
        ESP_LOGW(Webserver::Name, "Unable to parse data from control tuning message");
        return PBRet::FAILURE;
    }
    
    cJSON* PGainJSON = cJSON_GetObjectItemCaseSensitive(msgData, "PGain");
    if (cJSON_IsNumber(PGainJSON)) {
        PGain = PGainJSON->valuedouble;
    } else {
        ESP_LOGW(Webserver::Name, "Unable to read PGain from control tuning message");
        return PBRet::FAILURE;
    }

    cJSON* IGainJSON = cJSON_GetObjectItemCaseSensitive(msgData, "IGain");
    if (cJSON_IsNumber(IGainJSON)) {
        IGain = IGainJSON->valuedouble;
    } else {
        ESP_LOGW(Webserver::Name, "Unable to read IGain from control tuning message");
        return PBRet::FAILURE;
    }

    cJSON* DGainJSON = cJSON_GetObjectItemCaseSensitive(msgData, "DGain");
    if (cJSON_IsNumber(DGainJSON)) {
        DGain = DGainJSON->valuedouble;
    } else {
        ESP_LOGW(Webserver::Name, "Unable to read DGain from control tuning message");
        return PBRet::FAILURE;
    }

    cJSON* setpointJSON = cJSON_GetObjectItemCaseSensitive(msgData, "Setpoint");
    if (cJSON_IsNumber(setpointJSON)) {
        setpoint = setpointJSON->valuedouble;
    } else {
        ESP_LOGW(Webserver::Name, "Unable to read setpoint from control tuning message");
        return PBRet::FAILURE;
    }

    cJSON* LPFCutoffJSON = cJSON_GetObjectItemCaseSensitive(msgData, "LPFCutoff");
    if (cJSON_IsNumber(LPFCutoffJSON)) {
        LPFCutoff = LPFCutoffJSON->valuedouble;
    } else {
        ESP_LOGW(Webserver::Name, "Unable to read lowpass filter cutoff from control tuning message");
        return PBRet::FAILURE;
    }

    std::shared_ptr<ControlTuning> msg = std::make_shared<ControlTuning> (setpoint, PGain, IGain, DGain, LPFCutoff);

    ESP_LOGI(Webserver::Name, "Broadcasting controller tuning");
    return MessageServer::broadcastMessage(msg);
    
    return PBRet::SUCCESS;
}

PBRet Webserver::_parseControlSettingsMessage(cJSON* msgRoot)
{
    ControlTuning ctrlTuning {};
    bool fanState, elementLow, elementHigh, prodPump, refluxPump;

    if (msgRoot == nullptr) {
        ESP_LOGW(Webserver::Name, "Unable to parse control settings message. cJSON object was null");
        return PBRet::FAILURE;
    }

    cJSON* msgData = cJSON_GetObjectItemCaseSensitive(msgRoot, "data");
    if (msgData == nullptr) {
        ESP_LOGW(Webserver::Name, "Unable to parse data from control settings message");
        return PBRet::FAILURE;
    }
    
    cJSON* fanStateJSON = cJSON_GetObjectItemCaseSensitive(msgData, "fanState");
    if (cJSON_IsNumber(fanStateJSON)) {
        fanState = fanStateJSON->valueint;
    } else {
        ESP_LOGW(Webserver::Name, "Unable to read fanState from control settings message");
        return PBRet::FAILURE;
    }

    cJSON* elementLowJSON = cJSON_GetObjectItemCaseSensitive(msgData, "elementLow");
    if (cJSON_IsNumber(elementLowJSON)) {
        elementLow = elementLowJSON->valueint;
    } else {
        ESP_LOGW(Webserver::Name, "Unable to read low power element state from control settings message");
        return PBRet::FAILURE;
    }

    cJSON* elementHighJSON = cJSON_GetObjectItemCaseSensitive(msgData, "elementHigh");
    if (cJSON_IsNumber(elementHighJSON)) {
        elementHigh = elementHighJSON->valueint;
    } else {
        ESP_LOGW(Webserver::Name, "Unable to read high power element state from control settings message");
        return PBRet::FAILURE;
    }

    cJSON* prodPumpJSON = cJSON_GetObjectItemCaseSensitive(msgData, "prodPump");
    if (cJSON_IsNumber(prodPumpJSON)) {
        prodPump = prodPumpJSON->valueint;
    } else {
        ESP_LOGW(Webserver::Name, "Unable to read product pump state from control settings message");
        return PBRet::FAILURE;
    }

    cJSON* refluxPumpJSON = cJSON_GetObjectItemCaseSensitive(msgData, "refluxPump");
    if (cJSON_IsNumber(refluxPumpJSON)) {
        refluxPump = refluxPumpJSON->valueint;
    } else {
        ESP_LOGW(Webserver::Name, "Unable to read reflux pump state from control settings message");
        return PBRet::FAILURE;
    }

    std::shared_ptr<ControlSettings> msg = std::make_shared<ControlSettings> (fanState, elementLow, elementHigh, prodPump, refluxPump);

    ESP_LOGI(Webserver::Name, "Broadcasting controller settings");
    return MessageServer::broadcastMessage(msg);
}

PBRet Webserver::_parseCommandMessage(cJSON* root)
{
    if (root == nullptr) {
        ESP_LOGW(Webserver::Name, "Unable to parse command message. cJSON object was null");
        return PBRet::FAILURE;
    }

    std::string subtypeStr {};

    cJSON* subtype = cJSON_GetObjectItemCaseSensitive(root, "subtype");
    if (subtype != nullptr) {
        subtypeStr = std::string(subtype->valuestring);
    } else {
        ESP_LOGW(Webserver::Name, "Unable to parse subtype from command message");
        return PBRet::FAILURE;
    }

    if (subtypeStr == Webserver::BroadcastDevices) {
        // Send message to trigger scan of sensor bus
        std::shared_ptr<SensorManagerCommand> msg = std::make_shared<SensorManagerCommand> (SensorManagerCmdType::BroadcastSensorsStart);
        ESP_LOGI(Webserver::Name, "Sending message to start sensor assign task");
        return MessageServer::broadcastMessage(msg);
    } else if (subtypeStr == Webserver::AssignSensor) {
        return _processAssignSensorMessage(root);
    } else {
        ESP_LOGW(Webserver::Name, "Unable to parse command message with subtype %s", subtypeStr.c_str());
        return PBRet::FAILURE;
    }
}

PBRet Webserver::_processAssignSensorMessage(cJSON* root)
{
    if (root == nullptr) {
        ESP_LOGW(Webserver::Name, "Unable to parse assign sensor message. cJSON object was null");
        return PBRet::FAILURE;
    }

    ESP_LOGI(Webserver::Name, "Decoding assign sensor message");

    cJSON* sensorData = cJSON_GetObjectItemCaseSensitive(root, "data");
    if (sensorData == nullptr) {
        ESP_LOGW(Webserver::Name, "Unable to parse sensor data from assign sensor message");
        return PBRet::FAILURE;
    }

    // TODO: Allow Ds18b20 initialization from JSON
    cJSON* sensorAddr = cJSON_GetObjectItemCaseSensitive(sensorData, "addr");
    if (sensorAddr == nullptr) {
        ESP_LOGW(Webserver::Name, "Unable to parse sensor address from assign sensor message");
        return PBRet::FAILURE;
    }

    // Read address from JSON
    OneWireBus_ROMCode romCode {};
    cJSON* byte = nullptr;
    int i = 0;
    cJSON_ArrayForEach(byte, sensorAddr)
    {
        if (cJSON_IsNumber(byte) == false) {
            ESP_LOGW(Webserver::Name, "Unable to decode sensor address. Address byte was not a number");
            return PBRet::FAILURE;
        }
        romCode.bytes[i++] = byte->valueint;
    }

    // Read assigned sensor task from JSON
    cJSON* task = cJSON_GetObjectItemCaseSensitive(sensorData, "task");
    if (task == nullptr) {
        ESP_LOGW(Webserver::Name, "Unable to parse assigned sensor task from assign sensor message");
        return PBRet::FAILURE;
    }
    SensorType sensorType = PBOneWire::mapSensorIDToType(task->valueint);

    std::shared_ptr<AssignSensorCommand> msg = std::make_shared<AssignSensorCommand> (romCode, sensorType);
    return MessageServer::broadcastMessage(msg);
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

PBRet Webserver::_requestControllerTuning(void)
{
    // Request controller tuning settings
    // 

    std::shared_ptr<ControllerDataRequest> msg = std::make_shared<ControllerDataRequest> (ControllerDataRequestType::Tuning);

    return MessageServer::broadcastMessage(msg);
}

PBRet Webserver::_requestControllerSettings(void)
{
    // Request controller settings
    // 

    std::shared_ptr<ControllerDataRequest> msg = std::make_shared<ControllerDataRequest> (ControllerDataRequestType::Settings);

    return MessageServer::broadcastMessage(msg);
}