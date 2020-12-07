#include "SensorManager.h"
#include "MessageDefs.h"
#include "esp_spiffs.h"
#include "Filesystem.h"
#include <fstream>
#include <sstream>
#include <iostream>

SensorManager::SensorManager(UBaseType_t priority, UBaseType_t stackDepth, BaseType_t coreID, const SensorManagerConfig& cfg)
    : Task(SensorManager::Name, priority, stackDepth, coreID)
{
    // Setup callback table
    _setupCBTable();

    // Initialize SensorManager
    if (_initFromParams(cfg) == PBRet::SUCCESS) {
        ESP_LOGI(SensorManager::Name, "SensorManager configured!");
        _configured = true;
    } else {
        ESP_LOGW(SensorManager::Name, "Unable to configure SensorManager");
    }
}

void SensorManager::taskMain(void)
{
    // Subscribe to messages
    std::set<MessageType> subscriptions = { 
        MessageType::General,
        MessageType::SensorManagerCmd,
        MessageType::AssignSensor
    };
    Subscriber sub(SensorManager::Name, _GPQueue, subscriptions);
    MessageServer::registerTask(sub);

    // Set update frequency
    const TickType_t timestep =  _cfg.dt * 1000 / portTICK_PERIOD_MS;      // TODO: Define method for converting time
    portTickType xLastWakeTime = xTaskGetTickCount();

    while (true) {
        // Retrieve data from the queue
        _processQueue();

        // Read temperature sensors
        TemperatureData Tdata {};
        if (_OWBus.readTempSensors(Tdata) != PBRet::SUCCESS) {
            ESP_LOGW(SensorManager::Name, "Unable to read temperature sensors");
            
            // Record fault
        }

        // Read flowmeters

        // Broadcast data
        _broadcastTemps(Tdata);

        vTaskDelayUntil(&xLastWakeTime, timestep);
    }
}

PBRet SensorManager::_generalMessageCB(std::shared_ptr<MessageBase> msg)
{
    std::shared_ptr<GeneralMessage> genMsg = std::static_pointer_cast<GeneralMessage>(msg);
    ESP_LOGI(SensorManager::Name, "Received general message: %s", genMsg->getMessage().c_str());  

    return PBRet::SUCCESS;
}

PBRet SensorManager::_commandMessageCB(std::shared_ptr<MessageBase> msg)
{
    SensorManagerCommand cmd = *std::static_pointer_cast<SensorManagerCommand>(msg);
    ESP_LOGI(SensorManager::Name, "Got SensorManagerCommand message");

    switch (cmd.getCommandType())
    {
        case (SensorManagerCmdType::BroadcastSensorsStart):
        {
            ESP_LOGI(SensorManager::Name, "Broadcasting sensor adresses");
            return _broadcastSensors();
        }
        case (SensorManagerCmdType::None):
        {
            ESP_LOGW(SensorManager::Name, "Received command None");
            break;
        }
        default:
        {
            ESP_LOGW(SensorManager::Name, "Unsupported command");
            break;
        }
    } 

    return PBRet::SUCCESS;
}

PBRet SensorManager::_assignSensorCB(std::shared_ptr<MessageBase> msg)
{
    AssignSensorCommand cmd = *std::static_pointer_cast<AssignSensorCommand>(msg);
    ESP_LOGI(SensorManager::Name, "Got AssignSensorCommand message");

    const Ds18b20 sensor(cmd.getAddress(), DS18B20_RESOLUTION::DS18B20_RESOLUTION_11_BIT, _OWBus.getOWB());
    if (sensor.isConfigured() == false) {
        ESP_LOGW(SensorManager::Name, "Failed to create valid Ds18b20 sensor");
        return PBRet::FAILURE;
    }

    return _OWBus.setTempSensor(cmd.getSensorType(), sensor);
}

PBRet SensorManager::_setupCBTable(void)
{
    _cbTable = std::map<MessageType, queueCallback> {
        {MessageType::General, std::bind(&SensorManager::_generalMessageCB, this, std::placeholders::_1)},
        {MessageType::SensorManagerCmd, std::bind(&SensorManager::_commandMessageCB, this, std::placeholders::_1)},
        {MessageType::AssignSensor, std::bind(&SensorManager::_assignSensorCB, this, std::placeholders::_1)}
    };

    return PBRet::SUCCESS;
}

PBRet SensorManager::_initFromParams(const SensorManagerConfig& cfg)
{
    esp_err_t err = ESP_OK;

    _cfg = cfg;

    // Initialize PBOneWire bus
    _OWBus = PBOneWire(cfg.oneWireConfig);
    if (_OWBus.isConfigured() == false) {
        ESP_LOGE(SensorManager::Name, "OneWire bus is not configured");
        err = ESP_FAIL;
    }

    // Load saved devices
    if (_loadKnownDevices(SensorManager::FSBasePath, SensorManager::FSPartitionLabel) != PBRet::SUCCESS) {
        ESP_LOGW(SensorManager::Name, "No saved devices were found");
    }
    

    // Initialize flowrate sensors

    return err == ESP_OK ? PBRet::SUCCESS : PBRet::FAILURE;
}

PBRet SensorManager::_broadcastTemps(const TemperatureData& Tdata) const
{
    // Send a temperature data message to the queue
    std::shared_ptr<TemperatureData> msg = std::make_shared<TemperatureData> (Tdata);

    return MessageServer::broadcastMessage(msg);
}

PBRet SensorManager::checkInputs(const SensorManagerConfig& cfg)
{
    if (cfg.dt <= 0) {
        ESP_LOGE(SensorManager::Name, "Update time step %lf is invalid. SensorManager was not configured", cfg.dt);
        return PBRet::FAILURE;
    }

    // Check PBOneWire config
    if (PBOneWire::checkInputs(cfg.oneWireConfig) != PBRet::SUCCESS) {
        ESP_LOGE(SensorManager::Name, "Onewire bus config was invalid. SensorManager was not configured");
        return PBRet::FAILURE;
    }

    return PBRet::SUCCESS;
}

PBRet SensorManager::_loadKnownDevices(const char* basePath, const char* partitionLabel)
{
    // Nullptr check
    if ((basePath == nullptr) || (partitionLabel == nullptr)) {
        ESP_LOGW(SensorManager::Name, "Cannot load known devices. One or more input pointers were null");
        return PBRet::FAILURE;
    }

    // Mount filesystem
    Filesystem F(SensorManager::FSBasePath, SensorManager::FSPartitionLabel, 5, true);

    // Read JSON string from file
    std::ifstream configIn(SensorManager::deviceFile);
    if (configIn.good() == false) {
        ESP_LOGW(SensorManager::Name, "Devices file could not be opened");
        return PBRet::FAILURE;
    }

    // TODO: Stringstream is quite heavy for embedded. Might need to remove
    std::stringstream JSONBuffer;
    JSONBuffer << configIn.rdbuf();

    // Load JSON object
    // Just load one sensor for now. Package into proper methods for loading all sensors once
    // validated
    cJSON* configRoot = cJSON_Parse(JSONBuffer.str().c_str());
    if (configRoot == nullptr) {
        ESP_LOGW(SensorManager::Name, "Sensor config file was opened but could not be parsed");
        return PBRet::FAILURE;
    }

    // Load saved temperature sensors
    // No longer required to check for null, as this check is performed in 
    // cJSON_GetObjectItemCaseSensitive
    const cJSON* tempSensors = cJSON_GetObjectItemCaseSensitive(configRoot, "TempSensors");
    if (_loadTempSensorsFromJSON(tempSensors) != PBRet::SUCCESS) {
        ESP_LOGW(SensorManager::Name, "No temperature sensor config data was available");
    }

    // Load saved flowmeters
    const cJSON* flowmeters = cJSON_GetObjectItemCaseSensitive(configRoot, "Flowmeters");
    if (_loadFlowmetersFromJSON(flowmeters) != PBRet::SUCCESS) {
        ESP_LOGW(SensorManager::Name, "No flowmeter sensor config data was available");
    }

    cJSON_Delete(configRoot);
    return PBRet::SUCCESS;
}

PBRet SensorManager::_loadTempSensorsFromJSON(const cJSON* JSONTempSensors)
{
    if (JSONTempSensors == nullptr) {
        return PBRet::FAILURE;
    }

    Ds18b20 savedSensor {};

    // If there is saved head temp sensor in the config file, load it
    const cJSON* headTemp = cJSON_GetObjectItemCaseSensitive(JSONTempSensors, "headTemp");
    if (headTemp != nullptr) {
        savedSensor = Ds18b20(headTemp, _cfg.oneWireConfig.tempSensorResolution, _OWBus.getOWB());
        if (savedSensor.isConfigured() && _OWBus.isAvailableSensor(savedSensor)) {
            _OWBus.setTempSensor(SensorType::Head, savedSensor);
            ESP_LOGI(SensorManager::Name, "Loaded saved head temp sensor from file");
        }
    }

    // Load reflux out temp sensor
    const cJSON* refluxTemp = cJSON_GetObjectItemCaseSensitive(JSONTempSensors, "refluxTemp");
    if (refluxTemp != nullptr) {
        savedSensor = Ds18b20(refluxTemp, _cfg.oneWireConfig.tempSensorResolution, _OWBus.getOWB());
        if (savedSensor.isConfigured() && _OWBus.isAvailableSensor(savedSensor)) {
            _OWBus.setTempSensor(SensorType::Reflux, savedSensor);
            ESP_LOGI(SensorManager::Name, "Loaded reflux outflow temp sensor from file");
        }
    }

    // TODO: Load others

    return PBRet::SUCCESS;
}

PBRet SensorManager::_loadFlowmetersFromJSON(const cJSON* JSONFlowmeters)
{
    if (JSONFlowmeters == nullptr) {
        return PBRet::FAILURE;
    }

    // TODO: Implement

    return PBRet::SUCCESS;
}

PBRet SensorManager::_writeSensorConfigToFile(void) const
{
    // Write the current sensor configuration to JSON
    ESP_LOGI(SensorManager::Name, "Writing sensor configuration data to file");

    std::string JSONstr;
    if (_OWBus.serialize(JSONstr) != PBRet::SUCCESS) {
        ESP_LOGW(SensorManager::Name, "Failed to write sensor config to file");
        return PBRet::FAILURE;
    }
    
    // Mount filesystem
    Filesystem F(SensorManager::FSBasePath, SensorManager::FSPartitionLabel, 5, true);
    if (F.isOpen() == false) {
        ESP_LOGW(SensorManager::Name, "Failed to mount filesystem. Sensor data was not written to file");
        return PBRet::FAILURE;
    }

    std::ofstream outFile(SensorManager::deviceFile);
    if (outFile.is_open() == false) {
        ESP_LOGE(SensorManager::Name, "Failed to open file for writing. Sensor data was not written to file");
        return PBRet::FAILURE;
    }

    // Write JSON string out to file
    outFile << JSONstr;

    ESP_LOGI(SensorManager::Name, "Sensor configuration data successfully written to file");
    return PBRet::SUCCESS;
}

PBRet SensorManager::_printConfigFile(void) const
{
    // Print config JSON file to the console. Assumes filesystem is
    // mounted

    ESP_LOGI(SensorManager::Name, "Reading config file");

    std::ifstream JSONstream(SensorManager::deviceFile);
    if (JSONstream.good() == false) {
        ESP_LOGW(SensorManager::Name, "Failed to open config file for reading");
        return PBRet::FAILURE;
    }

    std::stringstream JSONBuffer;
    JSONBuffer << JSONstream.rdbuf();

    printf("%s\n", JSONBuffer.str().c_str());

    return PBRet::SUCCESS;
}

PBRet SensorManager::_broadcastSensors(void)
{
    // Scan OneWire bus for available sensors and advertise
    // addresses
    return _OWBus.broadcastAvailableDevices();
}