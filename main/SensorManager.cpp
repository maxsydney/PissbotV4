#include "SensorManager.h"
#include "esp_spiffs.h"
#include "Filesystem.h"
#include "Thermo.h"
#include "ABVTables.h"
#include "IO/Writable.h"
#include <fstream>
#include <sstream>
#include <iostream>

SensorManager::SensorManager(UBaseType_t priority, UBaseType_t stackDepth, BaseType_t coreID, const SensorManagerConfig& cfg)
    : Task(SensorManager::Name, priority, stackDepth, coreID), _refluxFlowmeter(cfg.refluxFlowConfig), 
      _productFlowmeter(cfg.productFlowConfig)
{
    // Setup callback table
    _setupCBTable();

    // Set message ID
    Task::_ID = MessageOrigin::SensorManager;

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
    std::set<PBMessageType> subscriptions = { 
        PBMessageType::SensorManagerCommand,
        PBMessageType::AssignSensor
    };
    Subscriber sub(SensorManager::Name, _GPQueue, subscriptions);
    MessageServer::registerTask(sub);

    // Set update frequency
    const TickType_t timestep =  _cfg.dt * 1000 / portTICK_PERIOD_MS;
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
        FlowrateData flowData {};
        if (_refluxFlowmeter.readMassFlowrate(esp_timer_get_time(), flowData.mutable_refluxFlowrate().get()) != PBRet::SUCCESS) {
            ESP_LOGW(SensorManager::Name, "Unable to read reflux flowmeter");
        }

        // Compute ABV
        ConcentrationData concData {};
        if (_estimateABV(Tdata, concData) != PBRet::SUCCESS) {
            ESP_LOGW(SensorManager::Name, "Unable to estimate ABV");
        }

        // Broadcast data
        _broadcastTemps(Tdata);
        _broadcastFlowrates(flowData);
        _broadcastConcentrations(concData);

        vTaskDelayUntil(&xLastWakeTime, timestep);
    }
}

PBRet SensorManager::_commandMessageCB(std::shared_ptr<PBMessageWrapper> msg)
{
    SensorManagerCommandMessage cmd {};
    if (MessageServer::unwrap(*msg, cmd) != PBRet::SUCCESS) {
        ESP_LOGW(SensorManager::Name, "Failed to decode SensorManagerCommand");
        return PBRet::FAILURE;
    } 
    ESP_LOGI(SensorManager::Name, "Got SensorManagerCommand message");

    switch (cmd.get_cmdType())
    {
        case (SensorManagerCmdType::CMD_BROADCAST_SENSORS):
        {
            ESP_LOGI(SensorManager::Name, "Broadcasting sensor adresses");
            return _broadcastSensors();
        }
        case (SensorManagerCmdType::CMD_NONE):
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

PBRet SensorManager::_assignSensorCB(std::shared_ptr<PBMessageWrapper> msg)
{
    // Create a new sensor object and assign it to the requested task. Write
    // new sensor configuration to filesystem

    PBAssignSensorCommand sensorMsg {};
    if (MessageServer::unwrap(*msg, sensorMsg) != PBRet::SUCCESS) {
        ESP_LOGW(SensorManager::Name, "Failed to decode DS18B20Sensor message");
        return PBRet::FAILURE;
    } 

    // Create new sensor object
    OneWireBus_ROMCode romCode {};

    // Copy bytes into ROM code
    for (size_t i = 0; i < ROM_SIZE; i++)
    {
        romCode.bytes[i] = sensorMsg.address()[i];
    }

    // TODO: Check here if this is a known sensor, and load it's calibration if so
    //       Otherwise, initialize with default calibration

    // Use default calibration for now
    const double calibLinear = 1.0;
    const double calibOffset = 0.0;

    const Ds18b20Config config(romCode, calibLinear, calibOffset, DS18B20_RESOLUTION::DS18B20_RESOLUTION_10_BIT, _OWBus.getOWB());

    // TODO: Decide where this object should be created + use unique ptr
    std::shared_ptr<Ds18b20> sensor = std::make_shared<Ds18b20>(config);
    if (sensor->isConfigured() == false) {
        ESP_LOGW(SensorManager::Name, "Failed to create valid Ds18b20 sensor");
        return PBRet::FAILURE;
    }

    // Assign sensor to requested task
    if (_OWBus.setTempSensor(sensorMsg.get_role(), sensor) != PBRet::SUCCESS) {
        ESP_LOGW(SensorManager::Name, "Failed to assign sensor");
        return PBRet::FAILURE;
    }

    // TODO: Error handling here
    _writeSensorConfigToFile();

    ESP_LOGI(SensorManager::Name, "Successfully assigned sensor");
    return PBRet::SUCCESS;
}

PBRet SensorManager::_setupCBTable(void)
{
    _cbTable = std::map<PBMessageType, queueCallback> {
        {PBMessageType::SensorManagerCommand, std::bind(&SensorManager::_commandMessageCB, this, std::placeholders::_1)},
        {PBMessageType::AssignSensor, std::bind(&SensorManager::_assignSensorCB, this, std::placeholders::_1)}
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
    PBMessageWrapper wrapped = MessageServer::wrap(Tdata, PBMessageType::TemperatureData, _ID);

    return MessageServer::broadcastMessage(wrapped);
}

PBRet SensorManager::_broadcastFlowrates(const FlowrateData& flowData) const
{
    // Send a temperature data message to the queue
    PBMessageWrapper wrapped = MessageServer::wrap(flowData, PBMessageType::FlowrateData, _ID);

    return MessageServer::broadcastMessage(wrapped);
}

PBRet SensorManager::_broadcastConcentrations(const ConcentrationData& concData) const
{
    // Send a temperature data message to the queue
    PBMessageWrapper wrapped = MessageServer::wrap(concData, PBMessageType::ConcentrationData, _ID);

    return MessageServer::broadcastMessage(wrapped);
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

    // Check reflux flowmeter config
    {
        if (Flowmeter::checkInputs(cfg.refluxFlowConfig) != PBRet::SUCCESS) {
            ESP_LOGE(SensorManager::Name, "Reflux flowmeter config was invalid. SensorManager was not configured");
            return PBRet::FAILURE;
        }
    }

    // Check product flowmeter config
    {
        if (Flowmeter::checkInputs(cfg.productFlowConfig) != PBRet::SUCCESS) {
            ESP_LOGE(SensorManager::Name, "Reflux flowmeter config was invalid. SensorManager was not configured");
            return PBRet::FAILURE;
        }
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

    // Read serialized config from file
    std::ifstream configIn(SensorManager::assignedSensorFile, std::ios::binary);
    if (configIn.good() == false) {
        ESP_LOGW(SensorManager::Name, "Devices file could not be opened");
        return PBRet::FAILURE;
    }

    std::vector<uint8_t> bytes(
         (std::istreambuf_iterator<char>(configIn)),
         (std::istreambuf_iterator<char>()));

    Readable buffer {};
    for (uint8_t byte : bytes)
    {
        buffer.push(byte);
    }

    if (_OWBus.deserialize(buffer) != PBRet::SUCCESS)
    {
        ESP_LOGW(SensorManager::Name, "Failed to read saved sensors from file");
        return PBRet::FAILURE;
    }

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

    Writable buffer {};
    if (_OWBus.serialize(buffer) != PBRet::SUCCESS) {
        ESP_LOGW(SensorManager::Name, "Failed to write sensor config to file");
        return PBRet::FAILURE;
    }
    
    // Mount filesystem
    Filesystem F(SensorManager::FSBasePath, SensorManager::FSPartitionLabel, 5, true);
    if (F.isOpen() == false) {
        ESP_LOGW(SensorManager::Name, "Failed to mount filesystem. Sensor data was not written to file");
        return PBRet::FAILURE;
    }

    std::ofstream outFile(SensorManager::assignedSensorFile, std::ios::out | std::ios::binary);
    if (outFile.is_open() == false) {
        ESP_LOGE(SensorManager::Name, "Failed to open file for writing. Sensor data was not written to file");
        return PBRet::FAILURE;
    }

    // Write serialized to file
    // TODO: C++ casting
    outFile.write((const char*) buffer.get_buffer(), buffer.get_size());

    ESP_LOGI(SensorManager::Name, "Sensor configuration data successfully written to file");
    return PBRet::SUCCESS;
}

PBRet SensorManager::_broadcastSensors(void)
{
    // Scan OneWire bus for available sensors and advertise
    // addresses
    return _OWBus.broadcastAvailableDevices();
}

PBRet SensorManager::_estimateABV(const TemperatureData &TData, ConcentrationData& concData) const
{
    // Estimate the vapour (head) and boiler alcohol concentrations and broadcast them
    // to web interface
    //
    // TODO: Not sure that this is where this method should live permanently

    // Only do lookup if temperature is within interpolation range
    if ((TData.get_headTemp() > ABVTables::MIN_TEMPERATURE) && (TData.get_headTemp() < ABVTables::MAX_TEMPERATURE)) {
        concData.set_vapourConcentration(Thermo::computeVapourABVLookup(TData.get_headTemp()));
        concData.set_boilerConcentration(Thermo::computeLiquidABVLookup(TData.get_boilerTemp()));
    }

    return PBRet::SUCCESS;
}