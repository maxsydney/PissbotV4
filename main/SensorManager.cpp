#include "SensorManager.h"
#include "MessageDefs.h"
#include "esp_spiffs.h"
#include "Filesystem.h"
#include <fstream>
#include <sstream>
#include <iostream>

SensorManager::SensorManager(UBaseType_t priority, UBaseType_t stackDepth, BaseType_t coreID, const SensorManagerConfig& cfg)
    : Task(SensorManager::Name, priority, stackDepth, coreID), _refluxFlowmeter(cfg.refluxFlowConfig), _productFlowmeter(cfg.productFlowConfig)
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
        FlowrateData flowData(0.0, 0.0);
        if (_refluxFlowmeter.readFlowrate(esp_timer_get_time(), flowData.refluxFlowrate) != PBRet::SUCCESS) {
            ESP_LOGW(SensorManager::Name, "Unable to read reflux flowmeter");
        }
        ESP_LOGI(SensorManager::Name, "Flowrate: %f (L/s)", flowData.refluxFlowrate);
        ESP_LOGI(SensorManager::Name, "Count: %d", _refluxFlowmeter.getFreqCounter());

        // Broadcast data
        _broadcastTemps(Tdata);
        _broadcastFlowrates(flowData);

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
    // Create a new sensor object and assign it to the requested task. Write
    // new sensor configuration to filesystem

    AssignSensorCommand cmd = *std::static_pointer_cast<AssignSensorCommand>(msg);
    ESP_LOGI(SensorManager::Name, "Assigning new temperature sensor");

    // Create new sensor object
    const Ds18b20 sensor(cmd.getAddress(), DS18B20_RESOLUTION::DS18B20_RESOLUTION_11_BIT, _OWBus.getOWB());
    if (sensor.isConfigured() == false) {
        ESP_LOGW(SensorManager::Name, "Failed to create valid Ds18b20 sensor");
        return PBRet::FAILURE;
    }

    // Assign sensor to requested task
    if (_OWBus.setTempSensor(cmd.getSensorType(), sensor) != PBRet::SUCCESS) {
        ESP_LOGW(SensorManager::Name, "Failed to assign sensor");
        return PBRet::FAILURE;
    }

    // Write new sensor configuration to file
    return _writeSensorConfigToFile();
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
    if (_initOneWireBus(cfg) != PBRet::SUCCESS) {
        // Err message printed in _initOneWireBus
        err += ESP_FAIL;
    }

    // Load saved devices
    if (_loadKnownDevices(SensorManager::FSBasePath, SensorManager::FSPartitionLabel) != PBRet::SUCCESS) {
        ESP_LOGW(SensorManager::Name, "No saved devices were found");
    }
    
    // Check flowrate sensors are configured
    if (_refluxFlowmeter.isConfigured() == false) {
        ESP_LOGW(SensorManager::Name, "Reflux flowmeter was not configured");
        err += ESP_FAIL;
    }

    if (_productFlowmeter.isConfigured() == false) {
        ESP_LOGW(SensorManager::Name, "Product flowmeter was not configured");
        err += ESP_FAIL;
    }
    
    return err == ESP_OK ? PBRet::SUCCESS : PBRet::FAILURE;
}

PBRet SensorManager::_initOneWireBus(const SensorManagerConfig &cfg)
{
    _OWBus = PBOneWire(cfg.oneWireConfig);
    if (_OWBus.isConfigured() == false) {
        ESP_LOGE(SensorManager::Name, "OneWire bus is not configured");
        return PBRet::FAILURE;
    }

    return PBRet::SUCCESS;
}

PBRet SensorManager::_broadcastTemps(const TemperatureData& Tdata) const
{
    // Send a temperature data message to the queue
    std::shared_ptr<TemperatureData> msg = std::make_shared<TemperatureData> (Tdata);

    return MessageServer::broadcastMessage(msg);
}

PBRet SensorManager::_broadcastFlowrates(const FlowrateData& flowData) const
{
    // Send a temperature data message to the queue
    std::shared_ptr<FlowrateData> msg = std::make_shared<FlowrateData> (flowData);

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
    // No longer required to check for null, as this check is performed in cJSON_GetObjectItemCaseSensitive
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
    const cJSON* headTemp = cJSON_GetObjectItemCaseSensitive(JSONTempSensors, SensorManager::HeadTempSensorKey);
    if (headTemp != nullptr) {
        savedSensor = Ds18b20(headTemp, _cfg.oneWireConfig.tempSensorResolution, _OWBus.getOWB());
        if (savedSensor.isConfigured() && _OWBus.isAvailableSensor(savedSensor)) {
            _OWBus.setTempSensor(SensorType::Head, savedSensor);
            ESP_LOGI(SensorManager::Name, "Loaded head temp sensor from file");
        } else {
            ESP_LOGW(SensorManager::Name, "Unable to head temp sensor object from file");
        }
    }

    // Load reflux out temp sensor
    const cJSON* refluxTemp = cJSON_GetObjectItemCaseSensitive(JSONTempSensors, SensorManager::RefluxTempSensorKey);
    if (refluxTemp != nullptr) {
        savedSensor = Ds18b20(refluxTemp, _cfg.oneWireConfig.tempSensorResolution, _OWBus.getOWB());
        if (savedSensor.isConfigured() && _OWBus.isAvailableSensor(savedSensor)) {
            _OWBus.setTempSensor(SensorType::Reflux, savedSensor);
            ESP_LOGI(SensorManager::Name, "Loaded reflux outflow temp sensor from file");
        } else {
            ESP_LOGW(SensorManager::Name, "Unable to reflux temp sensor object from file");
        }
    }

    // Load product out temp sensor
    const cJSON* productTemp = cJSON_GetObjectItemCaseSensitive(JSONTempSensors, SensorManager::ProductTempSensorKey);
    if (productTemp != nullptr) {
        savedSensor = Ds18b20(productTemp, _cfg.oneWireConfig.tempSensorResolution, _OWBus.getOWB());
        if (savedSensor.isConfigured() && _OWBus.isAvailableSensor(savedSensor)) {
            _OWBus.setTempSensor(SensorType::Product, savedSensor);
            ESP_LOGI(SensorManager::Name, "Loaded product outflow temp sensor from file");
        } else {
            ESP_LOGW(SensorManager::Name, "Unable to product temp sensor object from file");
        }
    }

    // Load radiator temp sensor
    const cJSON* radiatorTemp = cJSON_GetObjectItemCaseSensitive(JSONTempSensors, SensorManager::RadiatorTempSensorKey);
    if (radiatorTemp != nullptr) {
        savedSensor = Ds18b20(radiatorTemp, _cfg.oneWireConfig.tempSensorResolution, _OWBus.getOWB());
        if (savedSensor.isConfigured() && _OWBus.isAvailableSensor(savedSensor)) {
            _OWBus.setTempSensor(SensorType::Radiator, savedSensor);
            ESP_LOGI(SensorManager::Name, "Loaded radiator temp sensor from file");
        } else {
            ESP_LOGW(SensorManager::Name, "Unable to radiator temp sensor object from file");
        }
    }

    // Load boiler temp sensor
    const cJSON* boilerTemp = cJSON_GetObjectItemCaseSensitive(JSONTempSensors, SensorManager::BoilerTempSensorKey);
    if (boilerTemp != nullptr) {
        savedSensor = Ds18b20(boilerTemp, _cfg.oneWireConfig.tempSensorResolution, _OWBus.getOWB());
        if (savedSensor.isConfigured() && _OWBus.isAvailableSensor(savedSensor)) {
            _OWBus.setTempSensor(SensorType::Boiler, savedSensor);
            ESP_LOGI(SensorManager::Name, "Loaded boiler temp sensor from file");
        } else {
            ESP_LOGW(SensorManager::Name, "Unable to boiler temp sensor object from file");
        }
    }

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

PBRet SensorManager::loadFromJSON(SensorManagerConfig& cfg, const cJSON* cfgRoot)
{
    // Load SensorManagerConfig struct from JSON
    if (cfgRoot == nullptr) {
        ESP_LOGW(SensorManager::Name, "cfgRoot was null");
        return PBRet::FAILURE;
    }

    // Get SensorManager dt
    cJSON* dtNode = cJSON_GetObjectItem(cfgRoot, "dt");
    if (cJSON_IsNumber(dtNode)) {
        cfg.dt = dtNode->valuedouble;
    } else {
        ESP_LOGI(SensorManager::Name, "Unable to read SensorManager dt from JSON");
        return PBRet::FAILURE;
    }

    // Get OneWireBus configuration
    cJSON* OWBNode = cJSON_GetObjectItem(cfgRoot, "oneWireConfig");
    if (PBOneWire::loadFromJSON(cfg.oneWireConfig, OWBNode) != PBRet::SUCCESS) {
        ESP_LOGI(SensorManager::Name, "Unable to read OneWire bus config from JSON");
        return PBRet::FAILURE;
    }

    // Get reflux flowmeter configuration
    cJSON* refluxFlowNode = cJSON_GetObjectItem(cfgRoot, "refluxFlowmeterConfig");
    if (Flowmeter::loadFromJSON(cfg.refluxFlowConfig, refluxFlowNode) != PBRet::SUCCESS) {
        ESP_LOGI(SensorManager::Name, "Unable to read reflux flowmeter config from JSON");
        return PBRet::FAILURE;
    }

    // Get product flowmeter configuration
    cJSON* prodFlowNode = cJSON_GetObjectItem(cfgRoot, "productFlowmeterConfig");
    if (Flowmeter::loadFromJSON(cfg.productFlowConfig, prodFlowNode) != PBRet::SUCCESS) {
        ESP_LOGI(SensorManager::Name, "Unable to read product flowmeter config from JSON");
        return PBRet::FAILURE;
    }

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

    Filesystem F(SensorManager::FSBasePath, SensorManager::FSPartitionLabel, 5, false);

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

PBRet FlowrateData::serialize(std::string& JSONStr) const
{
    // Serialize the FlowrateDate object into a JSON string
    cJSON* root = cJSON_CreateObject();
    if (root == nullptr) {
        ESP_LOGW(FlowrateData::Name, "Unable to create root JSON object");
        return PBRet::FAILURE;
    }

    // Add reflux flowrate
    cJSON* refluxNode = cJSON_CreateNumber(refluxFlowrate);
    if (refluxNode == nullptr) {
        ESP_LOGW(FlowrateData::Name, "Error creating reflux flowrate JSON object");
        cJSON_Delete(root);
        return PBRet::FAILURE;
    }
    cJSON_AddItemToObject(root, FlowrateData::RefluxFlowrateStr, refluxNode);

    // Add product flowrate
    cJSON* productNode = cJSON_CreateNumber(productFlowrate);
    if (productNode == nullptr) {
        ESP_LOGW(FlowrateData::Name, "Error creating product flowrate JSON object");
        cJSON_Delete(root);
        return PBRet::FAILURE;
    }
    cJSON_AddItemToObject(root, FlowrateData::ProductFlowrateStr, productNode);

    // Add timestamp
    cJSON* timestampNode = cJSON_CreateNumber(_timeStamp);
    if (timestampNode == nullptr) {
        ESP_LOGW(FlowrateData::Name, "Error creating timestamp JSON object");
        cJSON_Delete(root);
        return PBRet::FAILURE;
    }
    cJSON_AddItemToObject(root, MessageBase::TimeStampStr, timestampNode);

    // Copy JSON to string. cJSON requires printing to a char* pointer. Copy into
    // std::string and free memory to avoid memory leak
    char* stringPtr = cJSON_Print(root);
    JSONStr = std::string(stringPtr);
    cJSON_Delete(root);
    free(stringPtr);

    return PBRet::SUCCESS;
}